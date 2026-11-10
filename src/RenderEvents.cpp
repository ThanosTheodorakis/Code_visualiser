// RenderEvents.cpp - Conversion between FlowEvent and the run file format.

#include "RenderEvents.h"

runfile::RunEvent toRunEvent(const FlowEvent& e)
{
    runfile::RunEvent r;
    r.type = static_cast<int>(e.type);
    r.takenRight = e.takenRight;
    r.duration = e.duration;
    r.kind = static_cast<int>(e.kind);
    r.forkKind = static_cast<int>(e.forkKind);
    r.forkId = e.forkId;
    r.loopId = e.loopId;
    r.label = e.label;
    r.endLabel = e.endLabel;
    r.armLeft = e.armLeft;
    r.armRight = e.armRight;
    r.name = e.name;
    return r;
}

FlowEvent fromRunEvent(const runfile::RunEvent& r)
{
    FlowEvent e;
    e.type = static_cast<FlowEventType>(r.type);
    e.takenRight = r.takenRight;
    e.duration = r.duration;
    e.kind = static_cast<FlowKind>(r.kind);
    e.forkKind = static_cast<ForkKind>(r.forkKind);
    e.forkId = r.forkId;
    e.loopId = r.loopId;
    e.label = r.label;
    e.endLabel = r.endLabel;
    e.armLeft = r.armLeft;
    e.armRight = r.armRight;
    e.name = r.name;
    return e;
}
