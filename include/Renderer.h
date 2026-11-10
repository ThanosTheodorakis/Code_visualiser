// Renderer.h - The Ogre application that draws and animates the flowchart.

#pragma once

#include "Ogre.h"
#include "OgreApplicationContext.h"
#include "OgreRTShaderSystem.h"

#include <deque>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "Anim.h"
#include "AnimPlanner.h"
#include "Backlog.h"
#include "CallStackTracker.h"
#include "CameraController.h"
#include "EventLog.h"
#include "FunctionBoxTracker.h"
#include "Hud.h"
#include "ImageExporter.h"
#include "InputController.h"
#include "PlayerControls.h"
#include "RenderEvents.h"
#include "configParser.h"

// The Ogre application that draws and animates the flowchart.
class Renderer : public OgreBites::ApplicationContext, private PlayerControls
{
public:
    // The one renderer of the process.
    static Renderer& getInstance();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Creates the scene, camera, interface and input once Ogre has started.
    void setup() override;

    using ForkId = int;
    using FlowKind = ::FlowKind;
    using ForkKind = ::ForkKind;

    // Adds a plain flow step.
    void enqueueFlowEvent(float durationSeconds = 1.2f, const std::string& label = "",
                          const std::string& endLabel = "", FlowKind kind = FlowKind::PLAIN);

    // How many loop passes are drawn before the rest are hidden.
    int maxIterationsShown() const { return configuration.getConfig().maxIterationsShown; }

    // Adds the start of a loop.
    void enqueueLoopEvent(float durationSeconds = 2.2f, const std::string& label = "", int loopId = -1);

    // Adds the end of a loop.
    void enqueueLoopEndEvent(int loopId = -1, float durationSeconds = 0.9f, const std::string& label = "");

    // Adds the start of a branch, switch or decision fork.
    void enqueueForkStartEvent(bool takenRight, ForkId forkId = -1, float durationSeconds = 2.0f,
                               const std::string& label = "", const std::string& armLeft = "",
                               const std::string& armRight = "", ForkKind forkKind = ForkKind::BRANCH);

    // Adds the point where a fork joins again.
    void enqueueForkEndEvent(ForkId forkId = -1, float durationSeconds = 0.9f, const std::string& label = "");

    // Adds an input step that waits for the user.
    void enqueueInputEvent(const std::string& label, float durationSeconds = 1.2f);
    // Marks the end of an input.
    void enqueueInputDoneEvent();
    // Adds an output step.
    void enqueueOutputEvent(const std::string& label, float durationSeconds = 1.0f);

    // Adds the START symbol.
    void enqueueProgramStartEvent();
    // Adds the END symbol.
    void enqueueProgramEndEvent();

    // Adds a function call and opens its box.
    void enqueueFunctionEnter(const std::string& label, const std::string& name = "");
    // Adds the return from a function and closes its box.
    void enqueueFunctionExit(const std::string& label = "");

    // Runs every frame on the render thread: plays events, advances animations, updates the interface and
    // publishes how many animations are unfinished, which is what lets the waiting program thread go on.
    bool frameRenderingQueued(const Ogre::FrameEvent& evt) override;

    // Turns the camera following on or off.
    void enableAutoScroll(bool enable);

    // Writes the recorded events to a run file.
    bool saveRun(const std::string& path);
    // Loads a run file and prepares it for replay.
    bool loadRun(const std::string& path);

    // Blocks the program thread while the renderer is behind or paused.
    void waitForCapacity() { backlog.waitForCapacity(); }

    // Lets a waiting program continue (used when closing).
    void releaseProducers() { backlog.release(); }

    // The gate that holds pause, speed and the queue limit.
    BacklogGate& gate() { return backlog; }

private:
    Renderer();

    virtual ~Renderer() {}

    // Player controls, implemented below.
    void togglePause() override;
    void restart() override;
    void stepBack() override;
    void stepForward() override;
    void speedUp() override;
    void slowDown() override;
    void exportImage() override;
    void saveRunNow() override;
    void toggleCallStack() override;
    void toggleLegend() override;
    void refreshControls() override;
    void quitApplication() override;

    configParser configuration;
    BacklogGate backlog;

    Ogre::SceneManager* scnMgr = nullptr;
    Ogre::Camera* cam = nullptr;
    Ogre::SceneNode* camNode = nullptr;

    LayoutParams layout;
    Ogre::Vector3 cursor{0, 500, 0};

    CameraController camera{layout};
    Hud hud;
    InputController input{*this, camera, hud};
    ImageExporter exporter;

    EventLog eventLog;
    AnimPlanner planner{layout};
    CallStackTracker callStack;
    FunctionBoxTracker boxes;

    bool replaying = false;
    std::size_t replayEnd = 0;
    // Rebuilds the chart instantly up to the given event.
    void jumpTo(std::size_t target);
    // Makes the camera follow the newest shape again.
    void followLatest(bool announce = true);
    // Number of events completely drawn.
    std::size_t shownEvents() const;
    // Number of events recorded.
    std::size_t totalEvents() const;
    // Works out the state shown by the status indicator.
    PlaybackStatus playbackStatus();
    // Refreshes the button labels.
    void updateControls();
    // Refreshes the call stack panel.
    void updateCallStackPanel();
    // Adds an event to the event log.
    void enqueueEventInternal(const FlowEvent& e);

    Ogre::SceneNode* worldNode = nullptr;
    std::vector<Ogre::ManualObject*> worldObjects;
    Ogre::ManualObject* cursorMarker = nullptr;
    int ogreHeadCount = 0;
    int layerCounter = 0;
    float lineWidth = 1.0f;
    float activeLineWidth = 1.7f;
    float arrowSize = 4.5f;
    float dashLength = 3.0f;
    float dashGap = 2.5f;

    // Adds an object to the chart node so it moves with the chart.
    void attachToWorld(Ogre::ManualObject* object);
    // Deletes everything drawn and returns to the start.
    void resetWorld();
    // Next small depth offset, so later shapes are drawn in front.
    float nextLayerZ();
    // Creates the drawable object of an animation if it has none.
    void ensureAnimObject(Anim& a, const char* prefix);
    // Draws a flow line up to its current progress.
    void drawFlow(Anim& a, bool settled);
    // Draws a loop circle up to its current progress.
    void drawLoop(Anim& a, float progress, bool settled);
    // Draws a fork; mode tells the stage of the animation.
    void drawFork(Anim& a, int mode, float progress);
    // Draws an input, output or START/END symbol.
    void drawIO(Anim& a, float progress, bool settled, bool waiting);
    // Writes the labels of both arms of a fork.
    void createForkArmLabels(const Anim& a);
    // Writes the start or end text of an animation next to its shape.
    Ogre::ManualObject* createLabelForAnim(const Anim& a, bool atStart, const std::string& textOverride = "");
    // Builds a text object from stroke font lines.
    Ogre::ManualObject* createTextObject(const std::string& text, float x, float y, float height,
                                         const Ogre::ColourValue& base, bool alignRight,
                                         bool countExtent = true);
    // Moves and pulses the marker at the cursor.
    void updateCursorMarker();

    // Starts a function box at a position.
    void openFunctionBox(const std::string& name, const Ogre::Vector3& at);
    // Closes the innermost function box.
    void closeFunctionBox(float bottomY, float returnX);
    // Tells the open boxes about a newly drawn area.
    void noteExtent(float x0, float y0, float x1, float y1);
    // Redraws one function box.
    void drawFunctionBox(FunctionBox& box);
    // Redraws the boxes that changed.
    void updateFunctionBoxes();

    std::vector<Anim> activeAnims;
    std::deque<Anim> pendingAnims;
    std::vector<std::pair<int, Ogre::Vector3>> forkJunctionStack;
    std::vector<LoopJunction> loopJunctionStack;
    std::vector<float> functionXStack;
    int inputsDone = 0;
    float drawnMinX = 0.0f;

    std::mutex sceneMutex;

    // Sets up an animation when it becomes active; false if it must wait. Dispatches to the init functions.
    bool initializeAnim(Anim& a);
    // One init function per kind of animation: they fix the shape's position from the cursor.
    bool initFunctionExit(Anim& a, const Ogre::Vector3& cursorPos);
    bool initForkReconnect(Anim& a, const Ogre::Vector3& cursorPos);
    bool initLoopReconnect(Anim& a, const Ogre::Vector3& cursorPos);
    bool initFlow(Anim& a, const Ogre::Vector3& cursorPos);
    bool initLoop(Anim& a, const Ogre::Vector3& cursorPos);
    bool initIO(Anim& a, const Ogre::Vector3& cursorPos);
    bool initFork(Anim& a, const Ogre::Vector3& cursorPos);
    // Turns unread events (up to limit) into pending animations.
    void drainEvents(std::size_t limit);
    // Starts the next animation and moves the active ones forward in time.
    void advanceAnimations(float deltaTime);
    // Makes the first pending animation active (if it can start) and creates its labels.
    void startNextAnimation();
    // One advance function per kind of animation: they draw activeAnims[index] and, when it is complete,
    // remove it from activeAnims. They return true while it is still active.
    bool advanceFlow(std::size_t index);
    bool advanceLoop(std::size_t index);
    bool advanceIO(std::size_t index);
    bool advanceFork(std::size_t index);
    // Completes all pending and active animations at once.
    void finishAnimations();
};
