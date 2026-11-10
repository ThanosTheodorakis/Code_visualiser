// CallStackTracker.cpp - Implementation of CallStackTracker.

#include "CallStackTracker.h"

void CallStackTracker::reset()
{
    frames_.clear();
    position_ = 0;
}

void CallStackTracker::advanceTo(const EventLog& log, std::size_t shown)
{
    log.inspect([&](const std::vector<FlowEvent>& events) {
        if (shown < position_ || position_ > events.size()) {
            frames_.clear();
            position_ = 0;
        }
        for (; position_ < shown && position_ < events.size(); ++position_) {
            const FlowEvent& e = events[position_];
            if (e.type == FlowEventType::FUNCTION_ENTER) {
                frames_.push_back(e.name.empty() ? e.label : e.name);
            } else if (e.type == FlowEventType::FUNCTION_EXIT && !frames_.empty()) {
                frames_.pop_back();
            }
        }
    });
}
