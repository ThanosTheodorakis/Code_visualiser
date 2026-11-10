// EventLog.h - Thread-safe list of all events of a run, with a read position for playback.

#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

#include "Backlog.h"
#include "RenderEvents.h"

// Thread-safe list of all events of a run, with a read position that playback moves.
// The program thread appends events and the render thread takes them; one mutex guards the list, the read
// position and the gate's counters, so the gate always matches the log.
class EventLog
{
public:
    // Adds an event at the end (called by the running program).
    void append(const FlowEvent& event, BacklogGate& gate);

    // Number of events recorded so far.
    std::size_t size() const;

    // Number of events already handed to the renderer.
    std::size_t readIndex() const;

    // Hands out the unread events up to limit; first receives the index of the first one.
    std::deque<FlowEvent> take(std::size_t limit, BacklogGate& gate, std::size_t& first);

    // Moves the read position back to the start.
    void rewind(BacklogGate& gate);

    // Replaces all events (when loading a run) and rewinds.
    void replace(std::vector<FlowEvent> events, BacklogGate& gate);

    // A copy of all events.
    std::vector<FlowEvent> snapshot() const;

    // Runs read on the events while holding the lock.
    template <class Reader> void inspect(Reader&& read) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        read(events_);
    }

private:
    mutable std::mutex mutex_;
    std::vector<FlowEvent> events_;
    std::size_t readIndex_ = 0;
};
