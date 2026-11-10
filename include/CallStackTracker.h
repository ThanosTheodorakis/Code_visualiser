// CallStackTracker.h - Works out which functions are open at a position in the event log.

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "EventLog.h"

// Works out which function calls are open at a given position in the event log.
class CallStackTracker
{
public:
    // Clears the tracked calls.
    void reset();

    // Moves the tracked position to shown events, replaying the log from the start when going back.
    void advanceTo(const EventLog& log, std::size_t shown);

    // Names of the open calls, outermost first.
    const std::vector<std::string>& frames() const { return frames_; }

private:
    std::vector<std::string> frames_;
    std::size_t position_ = 0;
};
