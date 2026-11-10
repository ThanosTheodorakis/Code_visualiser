// Renderer.cpp - Renderer setup, frame loop, player controls and events from the program.

#include "Renderer.h"

#include <algorithm>
#include <iostream>

#include <OgreMaterialManager.h>

#include "ProjectFolders.h"
#include "RunStore.h"

using namespace Ogre;

Renderer::Renderer()
    : OgreBites::ApplicationContext("Code_visualiser"),
      configuration(projectfolders::findConfig("config.cfg"))
{}

Renderer& Renderer::getInstance()
{
    static Renderer instance;
    return instance;
}

void Renderer::setup()
{
    OgreBites::ApplicationContext::setup();

    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName("BaseWhiteNoLighting");

    if (material && material->getNumTechniques() > 0 && material->getTechnique(0)->getNumPasses() > 0) {
        material->getTechnique(0)->getPass(0)->setVertexColourTracking(Ogre::TVC_DIFFUSE);
    }

    addInputListener(&input);
    getRoot()->addFrameListener(this);

    Root* root = getRoot();
    scnMgr = root->createSceneManager();
    worldNode = scnMgr->getRootSceneNode()->createChildSceneNode();

    RTShader::ShaderGenerator* shadergen = RTShader::ShaderGenerator::getSingletonPtr();

    if (shadergen) {
        shadergen->addSceneManager(scnMgr);
    }

    scnMgr->setAmbientLight(ColourValue(0.5f, 0.5f, 0.5f));

    Light* light = scnMgr->createLight("MainLight");
    SceneNode* lightNode = scnMgr->getRootSceneNode()->createChildSceneNode();

    lightNode->attachObject(light);
    lightNode->setPosition(20, 80, 50);

    camNode = scnMgr->getRootSceneNode()->createChildSceneNode();
    cam = scnMgr->createCamera("myCam");

    cam->setNearClipDistance(20);
    cam->setAutoAspectRatio(true);

    camNode->attachObject(cam);

    camera.attach(cam, camNode, getRenderWindow());

    camera.placeAtStart(cursor);

    Viewport* viewport = getRenderWindow()->addViewport(cam);
    viewport->setBackgroundColour(configuration.getConfig().backgroundColor);

    HudContext context;
    context.scnMgr = scnMgr;
    context.camNode = camNode;
    context.cam = cam;
    context.window = getRenderWindow();
    context.config = &configuration.getConfig();
    hud.create(context);

    exporter.attach(scnMgr, camNode, getRenderWindow(), cam);

    hud.updatePauseButton(backlog.paused());
    hud.updateLegend();
    updateControls();
    hud.updateStatus(playbackStatus());
    updateCallStackPanel();
}

bool Renderer::frameRenderingQueued(const Ogre::FrameEvent& event)
{
    drainEvents(static_cast<std::size_t>(-1));
    advanceAnimations(event.timeSinceLastFrame * backlog.timeScale());
    updateCallStackPanel();

    Vector3 currentCursor;
    {
        std::lock_guard<std::mutex> lock(sceneMutex);
        currentCursor = cursor;
    }
    camera.update(currentCursor);

    input.update(event.timeSinceLastFrame);

    updateFunctionBoxes();
    hud.updatePauseButton(backlog.paused());
    hud.updateLegend();
    updateControls();
    updateCursorMarker();
    hud.updateStatus(playbackStatus());

    backlog.publishAnimations(static_cast<int>(pendingAnims.size() + activeAnims.size()));

    return true;
}

void Renderer::enableAutoScroll(bool enable)
{
    camera.enableAutoScroll(enable);
}

void Renderer::updateControls()
{
    hud.updateControls(backlog.speed(), static_cast<int>(shownEvents()), static_cast<int>(totalEvents()));
}

void Renderer::updateCallStackPanel()
{
    callStack.advanceTo(eventLog, shownEvents());
    hud.updateCallStack(callStack.frames());
}

PlaybackStatus Renderer::playbackStatus()
{
    const std::size_t total = totalEvents();
    const std::size_t shown = shownEvents();

    if (replaying && shown >= replayEnd) {
        replaying = false;
    }

    bool ended = false;
    if (total > 0 && shown >= total) {
        eventLog.inspect([&](const std::vector<FlowEvent>& events) {
            ended = events.back().type == FlowEventType::PROGRAM_END;
        });
    }

    if (backlog.paused()) {
        return PlaybackStatus::Halted;
    }
    if (replaying) {
        return PlaybackStatus::Replaying;
    }
    if (ended) {
        return PlaybackStatus::Finished;
    }
    return PlaybackStatus::Live;
}

void Renderer::togglePause()
{
    backlog.togglePause();
    std::cout << (backlog.paused() ? "[player] paused\n" : "[player] resumed\n");
    hud.updatePauseButton(backlog.paused());
}

void Renderer::speedUp()
{
    backlog.speedUp();
    std::cout << "[player] speed x" << backlog.speed() << "\n";
}

void Renderer::slowDown()
{
    backlog.slowDown();
    std::cout << "[player] speed x" << backlog.speed() << "\n";
}

void Renderer::refreshControls()
{
    updateControls();
}

void Renderer::toggleCallStack()
{
    hud.toggleCallStack();
    updateCallStackPanel();
}

void Renderer::toggleLegend()
{
    hud.toggleLegend();
    hud.updateLegend();
}

void Renderer::quitApplication()
{
    if (getRoot()) {
        getRoot()->queueEndRendering();
    }
}

void Renderer::exportImage()
{
    exporter.exportView(cursorMarker, configuration.getConfig().backgroundColor);
}

void Renderer::followLatest(bool announce)
{
    camera.followLatest(announce);
}

std::size_t Renderer::totalEvents() const
{
    return eventLog.size();
}

std::size_t Renderer::shownEvents() const
{
    std::size_t position = eventLog.readIndex();
    for (const Anim& animation : activeAnims) {
        position = std::min(position, animation.eventIndex);
    }
    for (const Anim& animation : pendingAnims) {
        position = std::min(position, animation.eventIndex);
    }
    return position;
}

void Renderer::jumpTo(std::size_t target)
{
    replayEnd = std::max(replaying ? replayEnd : 0, totalEvents());
    replaying = true;
    resetWorld();
    drainEvents(std::min(target, totalEvents()));
    finishAnimations();

    camera.centreOn(cursor);
    followLatest(false);
}

void Renderer::restart()
{
    if (backlog.paused()) {
        togglePause();
    }
    jumpTo(0);
    std::cout << "[player] restarted\n";
}

void Renderer::stepBack()
{
    if (!backlog.paused()) {
        togglePause();
    }
    const std::size_t position = shownEvents();
    jumpTo(position > 0 ? position - 1 : 0);
}

void Renderer::stepForward()
{
    if (!backlog.paused()) {
        togglePause();
    }
    std::size_t target = shownEvents() + 1;
    eventLog.inspect([&](const std::vector<FlowEvent>& events) {
        while (target < events.size() && events[target].type == FlowEventType::INPUT_DONE) {
            ++target;
        }
    });
    jumpTo(target);
}

void Renderer::resetWorld()
{
    for (ManualObject* object : worldObjects) {
        scnMgr->destroyMovableObject(object);
    }
    worldObjects.clear();
    worldNode->removeAndDestroyAllChildren();

    activeAnims.clear();
    pendingAnims.clear();
    forkJunctionStack.clear();
    loopJunctionStack.clear();
    boxes.clear();
    functionXStack.clear();
    planner.reset();
    inputsDone = 0;
    layerCounter = 0;
    drawnMinX = 0.0f;
    cursorMarker = nullptr;

    {
        std::lock_guard<std::mutex> lock(sceneMutex);
        cursor = Vector3(0, 500, 0);
    }

    eventLog.rewind(backlog);
}

bool Renderer::saveRun(const std::string& path)
{
    return runstore::save(eventLog, path);
}

void Renderer::saveRunNow()
{
    saveRun(
        (projectfolders::runs() / (std::string("run_") + projectfolders::timestamp() + ".fvrun")).string());
}

bool Renderer::loadRun(const std::string& path)
{
    std::vector<FlowEvent> events;
    if (!runstore::load(path, events)) {
        return false;
    }

    const std::size_t count = events.size();
    eventLog.replace(std::move(events), backlog);

    resetWorld();
    callStack.reset();
    replaying = true;
    replayEnd = count;

    std::cout << "[load] loaded " << count << " events from " << path << "\n";
    return true;
}

void Renderer::enqueueEventInternal(const FlowEvent& event)
{
    eventLog.append(event, backlog);
}

void Renderer::enqueueFlowEvent(float durationSeconds, const std::string& label, const std::string& endLabel,
                                FlowKind kind)
{
    FlowEvent event;
    event.type = FlowEventType::FLOW_START;
    event.duration = durationSeconds;
    event.label = label;
    event.endLabel = endLabel;
    event.kind = kind;

    enqueueEventInternal(event);
}

void Renderer::enqueueFunctionEnter(const std::string& label, const std::string& name)
{
    FlowEvent event;
    event.type = FlowEventType::FUNCTION_ENTER;
    event.label = label;
    event.name = name;

    enqueueEventInternal(event);
}

void Renderer::enqueueFunctionExit(const std::string& label)
{
    FlowEvent event;
    event.type = FlowEventType::FUNCTION_EXIT;
    event.label = label;

    enqueueEventInternal(event);
}

void Renderer::enqueueProgramStartEvent()
{
    FlowEvent event;
    event.type = FlowEventType::PROGRAM_START;
    event.duration = 0.8f;

    enqueueEventInternal(event);
}

void Renderer::enqueueProgramEndEvent()
{
    FlowEvent event;
    event.type = FlowEventType::PROGRAM_END;
    event.duration = 0.8f;

    enqueueEventInternal(event);
}

void Renderer::enqueueInputEvent(const std::string& label, float durationSeconds)
{
    FlowEvent event;
    event.type = FlowEventType::INPUT_START;
    event.duration = durationSeconds;
    event.label = label;

    enqueueEventInternal(event);
}

void Renderer::enqueueInputDoneEvent()
{
    FlowEvent event;
    event.type = FlowEventType::INPUT_DONE;

    enqueueEventInternal(event);
}

void Renderer::enqueueOutputEvent(const std::string& label, float durationSeconds)
{
    FlowEvent event;
    event.type = FlowEventType::OUTPUT;
    event.duration = durationSeconds;
    event.label = label;

    enqueueEventInternal(event);
}

void Renderer::enqueueLoopEvent(float durationSeconds, const std::string& label, int loopId)
{
    FlowEvent event;
    event.type = FlowEventType::LOOP_START;
    event.duration = durationSeconds;
    event.label = label;
    event.loopId = loopId;

    enqueueEventInternal(event);
}

void Renderer::enqueueLoopEndEvent(int loopId, float durationSeconds, const std::string& label)
{
    FlowEvent event;
    event.type = FlowEventType::LOOP_END;
    event.duration = durationSeconds;
    event.loopId = loopId;
    event.label = label;

    enqueueEventInternal(event);
}

void Renderer::enqueueForkStartEvent(bool takenRight, int forkId, float durationSeconds,
                                     const std::string& label, const std::string& armLeft,
                                     const std::string& armRight, ForkKind forkKind)
{
    FlowEvent event;
    event.type = FlowEventType::FORK_START;
    event.forkKind = forkKind;
    event.armLeft = armLeft;
    event.armRight = armRight;
    event.takenRight = takenRight;
    event.duration = durationSeconds;
    event.label = label;
    event.forkId = forkId;

    enqueueEventInternal(event);
}

void Renderer::enqueueForkEndEvent(int forkId, float durationSeconds, const std::string& label)
{
    FlowEvent event;
    event.type = FlowEventType::FORK_END;
    event.duration = durationSeconds;
    event.forkId = forkId;
    event.label = label;

    enqueueEventInternal(event);
}
