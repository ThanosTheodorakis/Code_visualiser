// EventLog.cpp - Implementation of EventLog.

#include "EventLog.h"

#include <algorithm>

void EventLog::append(const FlowEvent& event, BacklogGate& gate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    gate.noteEnqueued();
    events_.push_back(event);
}

std::size_t EventLog::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
}

std::size_t EventLog::readIndex() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return readIndex_;
}

std::deque<FlowEvent> EventLog::take(std::size_t limit, BacklogGate& gate, std::size_t& first)
{
    std::deque<FlowEvent> taken;

    std::lock_guard<std::mutex> lock(mutex_);
    first = readIndex_;
    const std::size_t end = std::min(limit, events_.size());
    if (end > first) {
        taken.assign(events_.begin() + first, events_.begin() + end);
        readIndex_ = end;
    }
    gate.setQueued(static_cast<int>(events_.size() - readIndex_));
    return taken;
}

void EventLog::rewind(BacklogGate& gate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    readIndex_ = 0;
    gate.setQueued(static_cast<int>(events_.size()));
}

void EventLog::replace(std::vector<FlowEvent> events, BacklogGate& gate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    events_ = std::move(events);
    readIndex_ = 0;
    gate.setQueued(static_cast<int>(events_.size()));
}

std::vector<FlowEvent> EventLog::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
}
