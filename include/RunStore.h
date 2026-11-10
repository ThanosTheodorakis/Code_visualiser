// RunStore.h - Saving and loading a recorded run.

#pragma once

#include <string>
#include <vector>

#include "EventLog.h"

// Saving and loading a run file on disk.
namespace runstore {

// Writes all events of the log; false on failure.
bool save(const EventLog& log, const std::string& path);

// Reads a run file into events; false on failure.
bool load(const std::string& path, std::vector<FlowEvent>& events);

}
