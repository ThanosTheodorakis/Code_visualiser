// AnimPlanner.h - Turns one program event into the animations that draw it.

#pragma once

#include <deque>

#include "Anim.h"
#include "RenderEvents.h"
#include "configParser.h"

// Turns one program event into the animations that draw it.
class AnimPlanner
{
public:
    explicit AnimPlanner(const LayoutParams& layout) : layout_(layout) {}

    // Forgets the queued function depth (used when the chart is cleared).
    void reset() { functionDepthQueued_ = 0; }

    // Appends the animations for one event to out, using the planner of its type.
    // Returns true for an input-done event, which draws nothing.
    bool plan(const FlowEvent& event, const Ogre::Vector3& cursor, const ConfigData& configData,
              std::deque<Anim>& out);

private:
    // One planner per event type. Each appends the animations that draw the event to out.
    void planFlowStart(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);
    void planLoopStart(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);
    void planLoopEnd(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);
    void planForkStart(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);
    void planForkEnd(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);
    void planFunctionEnter(const FlowEvent& event, const Ogre::Vector3& cursor, const ConfigData& config,
                           std::deque<Anim>& out);
    void planFunctionExit(const FlowEvent& event, const Ogre::Vector3& cursor, const ConfigData& config,
                          std::deque<Anim>& out);
    void planInputStart(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);
    void planOutput(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);
    void planProgramStart(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);
    void planProgramEnd(const FlowEvent& event, const ConfigData& config, std::deque<Anim>& out);

    const LayoutParams& layout_;
    int functionDepthQueued_ = 0;
};
