// RunStore.cpp - Implementation of RunStore.

#include "RunStore.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace runstore {

bool save(const EventLog& log, const std::string& path)
{
    std::vector<runfile::RunEvent> events;
    log.inspect([&](const std::vector<FlowEvent>& all) {
        events.reserve(all.size());
        for (const FlowEvent& e : all) {
            events.push_back(toRunEvent(e));
        }
    });

    if (events.empty()) {
        std::cout << "[save] nothing to save yet\n";
        return false;
    }

    const std::filesystem::path target(path);
    if (target.has_parent_path()) {
        std::error_code folderError;
        std::filesystem::create_directories(target.parent_path(), folderError);
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        std::cout << "[save] cannot write " << path << "\n";
        return false;
    }
    runfile::write(out, events);
    out.close();
    if (!out) {
        std::cout << "[save] failed while writing " << path << "\n";
        return false;
    }

    std::cout << "[save] saved " << events.size() << " events to "
              << std::filesystem::absolute(target).string() << "\n";
    return true;
}

bool load(const std::string& path, std::vector<FlowEvent>& events)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cout << "[load] cannot open " << path << "\n";
        return false;
    }

    std::vector<runfile::RunEvent> raw;
    std::string error;
    if (!runfile::read(in, raw, error)) {
        std::cout << "[load] " << path << ": " << error << "\n";
        return false;
    }

    events.clear();
    events.reserve(raw.size());
    for (const runfile::RunEvent& e : raw) {
        events.push_back(fromRunEvent(e));
    }
    return true;
}

}
