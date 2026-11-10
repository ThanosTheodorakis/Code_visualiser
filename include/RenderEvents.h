// RenderEvents.h - The events the renderer plays and their conversion to and from run files.

#pragma once

#include <string>

#include "RunFile.h"

// Sub-kind of a plain flow event.
enum class FlowKind
{
    PLAIN,
    STATEMENT,
    ITERATION,
    BLOCK
};
// What a fork event stands for.
enum class ForkKind
{
    BRANCH,
    SWITCH,
    DECISION
};

// Kind of event the renderer plays.
enum class FlowEventType
{
    FLOW_START,
    LOOP_START,
    LOOP_END,
    FORK_START,
    FORK_END,
    FUNCTION_ENTER,
    FUNCTION_EXIT,
    INPUT_START,
    INPUT_DONE,
    OUTPUT,
    PROGRAM_START,
    PROGRAM_END
};

// One event of a run as the renderer stores and plays it.
struct FlowEvent
{
    FlowEventType type = FlowEventType::FLOW_START;
    bool takenRight = false;
    float duration = 1.0f;
    std::string label;
    std::string endLabel;
    std::string armLeft;
    std::string armRight;
    FlowKind kind = FlowKind::PLAIN;
    ForkKind forkKind = ForkKind::BRANCH;
    int forkId = -1;
    int loopId = -1;
    std::string name;
};

// Converts an event to its run file form.
runfile::RunEvent toRunEvent(const FlowEvent& e);
// Converts a run file event back.
FlowEvent fromRunEvent(const runfile::RunEvent& r);
