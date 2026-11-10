// RendererAnimation.cpp - Renderer part that plays events: places shapes in the chart and advances the animations.

#include "Renderer.h"

#include <algorithm>
#include <cmath>

#include "StrokeFont.h"

using namespace Ogre;

bool Renderer::initializeAnim(Anim& animation)
{
    Vector3 currentPosition;

    {
        std::lock_guard<std::mutex> lock(sceneMutex);
        currentPosition = cursor;
    }

    if (animation.isFunctionExit && !animation.hasGeometry) {
        return initFunctionExit(animation, currentPosition);
    }
    if (animation.reconnectForkId >= 0) {
        return initForkReconnect(animation, currentPosition);
    }
    if (animation.reconnectLoopId >= 0) {
        return initLoopReconnect(animation, currentPosition);
    }

    switch (animation.type) {
        case AnimType::FLOW:
            return initFlow(animation, currentPosition);
        case AnimType::LOOP:
            return initLoop(animation, currentPosition);
        case AnimType::IO:
            return initIO(animation, currentPosition);
        case AnimType::FORK:
            return initFork(animation, currentPosition);
    }
    return true;
}

bool Renderer::initFunctionExit(Anim& animation, const Vector3& currentPosition)
{
    animation.startPos = currentPosition;

    animation.endPos =
        Vector3(functionXStack.empty() ? currentPosition.x : functionXStack.back(), currentPosition.y, 0);

    animation.hasGeometry = true;
    return true;
}

bool Renderer::initForkReconnect(Anim& animation, const Vector3& currentPosition)
{
    auto iterator = std::find_if(
        forkJunctionStack.rbegin(), forkJunctionStack.rend(),
        [&](const std::pair<int, Vector3>& junction) { return junction.first == animation.reconnectForkId; });

    if (iterator == forkJunctionStack.rend()) {
        return false;
    }

    Vector3 origin = iterator->second;

    auto baseIterator = iterator.base();
    --baseIterator;

    forkJunctionStack.erase(baseIterator);

    animation.startPos = currentPosition;

    const float reconnectDrop = layout.flowStep() * 0.5f;

    animation.endPos = Vector3(origin.x, currentPosition.y - reconnectDrop, 0);

    animation.hasGeometry = true;
    animation.reconnectForkId = -1;

    return true;
}

bool Renderer::initLoopReconnect(Anim& animation, const Vector3& currentPosition)
{
    float step = layout.flowStep();

    auto iterator =
        std::find_if(loopJunctionStack.rbegin(), loopJunctionStack.rend(),
                     [&](const LoopJunction& junction) { return junction.id == animation.reconnectLoopId; });

    if (iterator == loopJunctionStack.rend()) {
        return false;
    }

    LoopJunction junction = *iterator;

    auto baseIterator = iterator.base();
    --baseIterator;

    loopJunctionStack.erase(baseIterator);

    const float loopRadiusYFactor = 0.35f;
    const float loopRadiusXFactor = 1.9f;

    float radiusY = step * loopRadiusYFactor;
    float radiusX = step * loopRadiusXFactor;

    Vector3 exitCenter(currentPosition.x, currentPosition.y + radiusY, 0);

    Vector3 exitLeft(exitCenter.x - radiusX, exitCenter.y, 0);

    Vector3 enterLeft(junction.center.x - junction.radiusX, junction.center.y, 0);

    const float leftmost = !boxes.anyOpen() ? drawnMinX : boxes.innermostMinX();

    const float laneX = std::min(std::min(exitLeft.x, enterLeft.x), leftmost - 14.0f * layout.drawScale) -
                        6.0f * layout.drawScale;

    drawnMinX = std::min(drawnMinX, laneX);
    noteExtent(laneX, exitLeft.y, laneX, enterLeft.y);

    animation.path = {exitLeft, Vector3(laneX, exitLeft.y, 0), Vector3(laneX, enterLeft.y, 0), enterLeft};

    animation.startPos = exitLeft;
    animation.endPos = enterLeft;
    animation.hasGeometry = true;
    animation.reconnectLoopId = -1;

    return true;
}

bool Renderer::initFlow(Anim& animation, const Vector3& currentPosition)
{
    if (!animation.hasGeometry) {
        animation.startPos = currentPosition;
        animation.endPos = Vector3(currentPosition.x, currentPosition.y - layout.flowStep(), 0);

        animation.hasGeometry = true;
    }

    return true;
}

bool Renderer::initLoop(Anim& animation, const Vector3& currentPosition)
{
    if (!animation.hasGeometry) {
        float step = layout.flowStep();

        const float loopRadiusYFactor = 0.35f;
        const float loopRadiusXFactor = 1.9f;

        animation.radiusY = step * loopRadiusYFactor;

        animation.radiusX = step * loopRadiusXFactor;

        animation.center = Vector3(currentPosition.x, currentPosition.y - animation.radiusY, 0);

        animation.radius = animation.radiusY;
        animation.hasGeometry = true;

        if (animation.loopId >= 0) {
            LoopJunction junction;
            junction.id = animation.loopId;
            junction.center = animation.center;
            junction.radiusX = animation.radiusX;
            junction.radiusY = animation.radiusY;

            loopJunctionStack.emplace_back(std::move(junction));
        }
    }

    return true;
}

bool Renderer::initIO(Anim& animation, const Vector3& currentPosition)
{
    if (!animation.hasGeometry) {
        float step = layout.flowStep();

        animation.radiusX = animation.terminator ? step * 1.1f : step * 1.6f;
        animation.radiusY = animation.terminator ? step * 0.34f : step * 0.38f;
        animation.center = Vector3(currentPosition.x, currentPosition.y - animation.radiusY, 0);

        animation.startPos = currentPosition;
        animation.endPos = Vector3(currentPosition.x, currentPosition.y - 2.0f * animation.radiusY, 0);

        animation.hasGeometry = true;
    }

    return true;
}

bool Renderer::initFork(Anim& animation, const Vector3& currentPosition)
{
    if (!animation.hasGeometry) {
        animation.startPos = currentPosition;
        animation.endPos = Vector3(currentPosition.x, currentPosition.y - layout.flowStep(), 0);

        animation.hasGeometry = true;
    }

    if (animation.forkId >= 0) {
        forkJunctionStack.emplace_back(animation.forkId, animation.endPos);
    }

    return true;
}

// Render thread: takes the events the program thread has added since the last frame (the read position
// moves on, and the gate learns how many are left) and plans their animations.
void Renderer::drainEvents(std::size_t limit)
{
    const ConfigData& configData = configuration.getConfig();

    std::size_t first = 0;
    std::deque<FlowEvent> localEvents = eventLog.take(limit, backlog, first);

    std::size_t processed = 0;
    std::size_t tagged = pendingAnims.size();
    auto tagNew = [&](std::size_t eventIndex) {
        for (; tagged < pendingAnims.size(); ++tagged) {
            pendingAnims[tagged].eventIndex = eventIndex;
        }
    };

    while (!localEvents.empty()) {
        FlowEvent currentEvent = std::move(localEvents.front());

        localEvents.pop_front();

        if (processed > 0) {
            tagNew(first + processed - 1);
        }
        ++processed;

        Vector3 currentPosition;

        {
            std::lock_guard<std::mutex> lock(sceneMutex);
            currentPosition = cursor;
        }

        if (planner.plan(currentEvent, currentPosition, configData, pendingAnims)) {
            ++inputsDone;
        }
    }

    if (processed > 0) {
        tagNew(first + processed - 1);
    }
}

void Renderer::advanceAnimations(float deltaTime)
{
    if (activeAnims.empty() && !pendingAnims.empty()) {
        startNextAnimation();
    }

    for (std::size_t index = 0; index < activeAnims.size();) {
        Anim& animation = activeAnims[index];

        animation.elapsed += deltaTime;

        bool stillActive = true;
        switch (animation.type) {
            case AnimType::FLOW:
                stillActive = advanceFlow(index);
                break;
            case AnimType::LOOP:
                stillActive = advanceLoop(index);
                break;
            case AnimType::IO:
                stillActive = advanceIO(index);
                break;
            case AnimType::FORK:
                stillActive = advanceFork(index);
                break;
        }

        if (stillActive) {
            ++index;
        }
    }
}

void Renderer::startNextAnimation()
{
    Anim& candidate = pendingAnims.front();

    if (!initializeAnim(candidate)) {
        return;
    }

    Anim next = std::move(candidate);
    pendingAnims.pop_front();
    activeAnims.push_back(std::move(next));

    if (activeAnims.back().opensBox) {
        openFunctionBox(activeAnims.back().boxName, activeAnims.back().startPos);

        functionXStack.push_back(activeAnims.back().startPos.x);
    }

    createLabelForAnim(activeAnims.back(), true);

    if (!activeAnims.back().centerText.empty()) {
        const Anim& shape = activeAnims.back();
        const float height = 10.0f * layout.drawScale;

        float width = 0.0f;
        for (const strokefont::Segment& segment : strokefont::layout(shape.centerText, height)) {
            width = std::max(width, std::max(segment.x1, segment.x2));
        }

        createTextObject(shape.centerText, shape.center.x - width * 0.5f, shape.center.y - height * 0.5f,
                         height, shape.color, false);
    }
}

bool Renderer::advanceFlow(std::size_t index)
{
    Anim& animation = activeAnims[index];

    drawFlow(animation, animation.elapsed >= animation.duration);

    if (animation.elapsed < animation.duration) {
        return true;
    }

    drawnMinX = std::min(drawnMinX, std::min(animation.startPos.x, animation.endPos.x));

    createLabelForAnim(animation, false);

    if (animation.closesBox) {
        closeFunctionBox(animation.endPos.y, animation.endPos.x);
    }

    if (animation.advanceCursorOnFinish) {
        std::lock_guard<std::mutex> lock(sceneMutex);

        cursor = animation.endPos;

        if (animation.isFunctionExit && !functionXStack.empty()) {
            functionXStack.pop_back();
        }
    }

    if (!pendingAnims.empty()) {
        Anim& next = pendingAnims.front();

        if (next.hasGeometry) {
            Vector3 oldStart = next.startPos;

            Vector3 oldEnd = next.endPos;

            Vector3 delta = oldEnd - oldStart;

            if (next.type == AnimType::FLOW) {
                next.startPos = cursor;
                next.endPos = cursor + delta;
                next.hasGeometry = true;
            } else if (next.type == AnimType::LOOP) {
                next.center = Vector3(cursor.x, cursor.y - next.radiusY, 0);

                next.hasGeometry = true;
            } else if (next.type == AnimType::FORK) {
                float step = layout.flowStep();

                next.startPos = cursor;
                next.endPos = Vector3(cursor.x, cursor.y - step, 0);

                next.hasGeometry = true;
            }
        }
    }

    activeAnims.erase(activeAnims.begin() + index);
    return false;
}

bool Renderer::advanceLoop(std::size_t index)
{
    Anim& animation = activeAnims[index];

    float progress = std::min(1.0f, animation.elapsed / animation.duration);

    drawLoop(animation, progress, progress >= 1.0f);

    if (progress < 1.0f) {
        return true;
    }

    drawnMinX = std::min(drawnMinX, animation.center.x - animation.radiusX);

    createLabelForAnim(animation, false);

    {
        std::lock_guard<std::mutex> lock(sceneMutex);

        float bottomY = animation.center.y - animation.radiusY;

        cursor = Vector3(animation.center.x, bottomY, 0);
    }

    activeAnims.erase(activeAnims.begin() + index);
    return false;
}

bool Renderer::advanceIO(std::size_t index)
{
    Anim& animation = activeAnims[index];

    float progress = std::min(1.0f, animation.elapsed / animation.duration);

    const bool drawn = progress >= 1.0f;
    const bool waiting = drawn && animation.ioWaits && inputsDone <= 0;

    drawIO(animation, progress, drawn && !waiting, waiting);

    if (waiting) {
        if (!animation.waitLabel) {
            animation.waitLabel = createLabelForAnim(animation, false, "waiting: type in the console");
        }

        return true;
    }
    if (!drawn) {
        return true;
    }

    if (animation.ioWaits) {
        --inputsDone;

        if (animation.waitLabel) {
            animation.waitLabel->setVisible(false);
        }
    }

    drawnMinX = std::min(drawnMinX, animation.center.x - animation.radiusX);

    createLabelForAnim(animation, false);

    {
        std::lock_guard<std::mutex> lock(sceneMutex);

        cursor = animation.endPos;
    }

    activeAnims.erase(activeAnims.begin() + index);
    return false;
}

bool Renderer::advanceFork(std::size_t index)
{
    Anim& animation = activeAnims[index];

    if (!animation.stageIncomingDone) {
        float incomingDuration = animation.duration * 0.45f;

        float stageProgress = std::min(1.0f, animation.elapsed / incomingDuration);

        drawFork(animation, 0, stageProgress);

        if (animation.elapsed >= incomingDuration) {
            animation.stageIncomingDone = true;
            animation.elapsed = 0.0f;

            float halfX = layout.xSpacing * layout.drawScale * 0.5f;

            float halfY = layout.ySpacing * layout.drawScale * 0.5f;

            animation.branchLeftTarget = Vector3(animation.endPos.x - halfX, animation.endPos.y - halfY, 0);

            animation.branchRightTarget = Vector3(animation.endPos.x + halfX, animation.endPos.y - halfY, 0);
        }

        return true;
    }

    float branchDuration = animation.duration * 0.55f;

    float stageProgress = std::min(1.0f, animation.elapsed / branchDuration);

    if (stageProgress < 1.0f) {
        drawFork(animation, 1, stageProgress);
        return true;
    }

    drawFork(animation, 2, 1.0f);
    createForkArmLabels(animation);
    drawnMinX = std::min(drawnMinX, animation.branchLeftTarget.x);

    Anim taken;
    taken.type = AnimType::FLOW;
    taken.duration = animation.duration * 0.6f;
    taken.color = animation.color;
    taken.startPos = animation.endPos;
    taken.endPos = animation.takenRight ? animation.branchRightTarget : animation.branchLeftTarget;
    taken.hasGeometry = true;
    taken.eventIndex = animation.eventIndex;

    activeAnims.erase(activeAnims.begin() + index);

    activeAnims.insert(activeAnims.begin() + index, std::move(taken));
    return false;
}

void Renderer::finishAnimations()
{
    int unchanged = 0;
    std::size_t lastPending = pendingAnims.size();
    std::size_t lastActive = activeAnims.size();

    while ((!pendingAnims.empty() || !activeAnims.empty()) && unchanged < 8) {
        advanceAnimations(1.0e4f);

        if (pendingAnims.size() == lastPending && activeAnims.size() == lastActive) {
            ++unchanged;
        } else {
            unchanged = 0;
            lastPending = pendingAnims.size();
            lastActive = activeAnims.size();
        }
    }
}
