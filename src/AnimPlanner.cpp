// AnimPlanner.cpp - Implementation of AnimPlanner.

#include "AnimPlanner.h"

#include <algorithm>

using namespace Ogre;

bool AnimPlanner::plan(const FlowEvent& event, const Vector3& cursor, const ConfigData& configData,
                       std::deque<Anim>& out)
{
    switch (event.type) {
        case FlowEventType::FLOW_START:
            planFlowStart(event, configData, out);
            break;
        case FlowEventType::LOOP_START:
            planLoopStart(event, configData, out);
            break;
        case FlowEventType::LOOP_END:
            planLoopEnd(event, configData, out);
            break;
        case FlowEventType::FORK_START:
            planForkStart(event, configData, out);
            break;
        case FlowEventType::FORK_END:
            planForkEnd(event, configData, out);
            break;
        case FlowEventType::FUNCTION_ENTER:
            planFunctionEnter(event, cursor, configData, out);
            break;
        case FlowEventType::FUNCTION_EXIT:
            planFunctionExit(event, cursor, configData, out);
            break;
        case FlowEventType::INPUT_START:
            planInputStart(event, configData, out);
            break;
        case FlowEventType::INPUT_DONE:
            return true;
        case FlowEventType::OUTPUT:
            planOutput(event, configData, out);
            break;
        case FlowEventType::PROGRAM_START:
            planProgramStart(event, configData, out);
            break;
        case FlowEventType::PROGRAM_END:
            planProgramEnd(event, configData, out);
            break;
    }
    return false;
}

void AnimPlanner::planFunctionEnter(const FlowEvent& currentEvent, const Vector3& cursor,
                                    const ConfigData& configData, std::deque<Anim>& out)
{
    ++functionDepthQueued_;

    float indent = layout_.xSpacing * layout_.drawScale * layout_.functionIndent;

    float newX = cursor.x + indent;

    float startY = cursor.y;
    float downY = startY - layout_.flowStep();

    Anim horizontal;
    horizontal.type = AnimType::FLOW;
    horizontal.duration = 0.08f;
    horizontal.color = configData.functionLineColor;
    horizontal.startPos = cursor;
    horizontal.endPos = Ogre::Vector3(newX, startY, 0);
    horizontal.hasGeometry = true;
    horizontal.advanceCursorOnFinish = true;
    horizontal.startText = currentEvent.label;
    horizontal.opensBox = true;
    horizontal.boxName = currentEvent.name;

    Anim vertical;
    vertical.type = AnimType::FLOW;
    vertical.duration = 0.12f;
    vertical.color = configData.functionLineColor;
    vertical.startPos = horizontal.endPos;
    vertical.endPos = Ogre::Vector3(newX, downY, 0);
    vertical.hasGeometry = true;
    vertical.advanceCursorOnFinish = true;

    out.push_back(std::move(horizontal));

    out.push_back(std::move(vertical));
}

void AnimPlanner::planFunctionExit(const FlowEvent& currentEvent, const Vector3& cursor,
                                   const ConfigData& configData, std::deque<Anim>& out)
{
    const bool hasPreviousX = functionDepthQueued_ > 0;
    if (hasPreviousX) {
        --functionDepthQueued_;
    }

    float dropAmount = layout_.flowStep() * 0.5f;

    float dropY = cursor.y - dropAmount;

    Anim drop;
    drop.type = AnimType::FLOW;
    drop.duration = 0.08f;
    drop.color = configData.functionLineColor;
    drop.startPos = cursor;
    drop.endPos = Ogre::Vector3(cursor.x, dropY, 0);

    drop.hasGeometry = true;
    drop.advanceCursorOnFinish = true;

    if (!hasPreviousX) {
        drop.endText = currentEvent.label;
        drop.closesBox = true;
    }

    out.push_back(std::move(drop));

    if (hasPreviousX) {
        Anim returnAnimation;
        returnAnimation.type = AnimType::FLOW;
        returnAnimation.duration = 0.12f;
        returnAnimation.color = configData.functionLineColor;
        returnAnimation.hasGeometry = false;
        returnAnimation.advanceCursorOnFinish = true;
        returnAnimation.isFunctionExit = true;
        returnAnimation.endText = currentEvent.label;
        returnAnimation.closesBox = true;

        out.push_back(std::move(returnAnimation));
    }
}

void AnimPlanner::planProgramStart(const FlowEvent& currentEvent, const ConfigData& configData,
                                   std::deque<Anim>& out)
{
    Anim animation;
    animation.type = AnimType::IO;
    animation.terminator = true;
    animation.duration = currentEvent.duration;
    animation.color = configData.terminalLineColor;
    animation.centerText = "START";

    out.push_back(std::move(animation));
}

void AnimPlanner::planProgramEnd(const FlowEvent& currentEvent, const ConfigData& configData,
                                 std::deque<Anim>& out)
{
    Anim animation;
    animation.type = AnimType::IO;
    animation.terminator = true;
    animation.duration = currentEvent.duration;
    animation.color = configData.terminalLineColor;
    animation.centerText = "END";

    out.push_back(std::move(animation));
}

void AnimPlanner::planInputStart(const FlowEvent& currentEvent, const ConfigData& configData,
                                 std::deque<Anim>& out)
{
    Anim animation;
    animation.type = AnimType::IO;
    animation.duration = currentEvent.duration;
    animation.color = configData.inputLineColor;
    animation.startText = currentEvent.label;
    animation.endText = "input received";
    animation.ioWaits = true;

    out.push_back(std::move(animation));
}

void AnimPlanner::planOutput(const FlowEvent& currentEvent, const ConfigData& configData,
                             std::deque<Anim>& out)
{
    Anim animation;
    animation.type = AnimType::IO;
    animation.duration = currentEvent.duration;
    animation.color = configData.outputLineColor;
    animation.startText = currentEvent.label;
    animation.endText = "";
    animation.ioWaits = false;

    out.push_back(std::move(animation));
}

void AnimPlanner::planFlowStart(const FlowEvent& currentEvent, const ConfigData& configData,
                                std::deque<Anim>& out)
{
    Anim animation;
    animation.type = AnimType::FLOW;
    animation.duration = currentEvent.duration;
    switch (currentEvent.kind) {
        case FlowKind::STATEMENT:
            animation.color = configData.statementLineColor;
            break;
        case FlowKind::ITERATION:
            animation.color = configData.iterationLineColor;
            break;
        default:
            animation.color = configData.flowLineColor;
            break;
    }
    animation.startText = currentEvent.label;
    animation.endText = currentEvent.endLabel;

    if (!currentEvent.label.empty()) {
        animation.label = currentEvent.label;
        animation.showLabel = true;
    }

    out.push_back(std::move(animation));
}

void AnimPlanner::planLoopStart(const FlowEvent& currentEvent, const ConfigData& configData,
                                std::deque<Anim>& out)
{
    Anim animation;
    animation.type = AnimType::LOOP;
    animation.duration = currentEvent.duration;
    animation.loopId = currentEvent.loopId;
    animation.color = configData.loopLineColor;
    animation.startText = currentEvent.label;

    if (!currentEvent.label.empty()) {
        animation.label = currentEvent.label;
        animation.showLabel = true;
    }

    out.push_back(std::move(animation));
}

void AnimPlanner::planLoopEnd(const FlowEvent& currentEvent, const ConfigData& configData,
                              std::deque<Anim>& out)
{
    Anim exitOval;
    exitOval.type = AnimType::LOOP;
    exitOval.duration = currentEvent.duration * 0.6f;

    exitOval.color = configData.loopLineColor;
    exitOval.showLabel = false;
    exitOval.endText = currentEvent.label;

    out.push_back(std::move(exitOval));

    Anim connector;
    connector.type = AnimType::FLOW;
    connector.duration = std::max(0.3f, currentEvent.duration * 0.6f);
    connector.color = configData.connectorLineColor;
    connector.hasGeometry = false;
    connector.reconnectLoopId = currentEvent.loopId;
    connector.advanceCursorOnFinish = false;
    connector.dashed = true;

    out.push_back(std::move(connector));
}

void AnimPlanner::planForkStart(const FlowEvent& currentEvent, const ConfigData& configData,
                                std::deque<Anim>& out)
{
    Anim incoming;
    incoming.type = AnimType::FLOW;
    incoming.duration = std::min(1.2f, currentEvent.duration);
    const Ogre::ColourValue forkColour =
        currentEvent.forkKind == ForkKind::SWITCH     ? configData.switchLineColor
        : currentEvent.forkKind == ForkKind::DECISION ? configData.decisionLineColor
                                                      : configData.branchLineColor;

    incoming.color = forkColour;
    incoming.startText = currentEvent.label;

    out.push_back(std::move(incoming));

    Anim fork;
    fork.type = AnimType::FORK;
    fork.duration = currentEvent.duration;
    fork.takenRight = currentEvent.takenRight;
    fork.armLeft = currentEvent.armLeft;
    fork.armRight = currentEvent.armRight;
    fork.forkId = currentEvent.forkId;
    fork.color = forkColour;

    if (!currentEvent.label.empty()) {
        fork.label = currentEvent.label;
        fork.showLabel = true;
    }

    out.push_back(std::move(fork));
}

void AnimPlanner::planForkEnd(const FlowEvent& currentEvent, const ConfigData& configData,
                              std::deque<Anim>& out)
{
    Anim marker;
    marker.type = AnimType::FLOW;
    marker.duration = currentEvent.duration;
    marker.color = configData.connectorLineColor;
    marker.dashed = true;
    marker.hasGeometry = false;
    marker.reconnectForkId = currentEvent.forkId;
    marker.endText = currentEvent.label;

    out.push_back(std::move(marker));
}
