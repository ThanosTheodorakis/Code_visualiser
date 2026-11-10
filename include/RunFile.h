// RunFile.h - Reading and writing the run file format.

#pragma once

#include <cstdlib>
#include <iomanip>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

// The text format of a recorded run.
namespace runfile {

// Version written to and required in the header.
constexpr int kVersion = 1;
// Largest valid event type.
constexpr int kMaxType = 11;
// Largest valid flow kind.
constexpr int kMaxKind = 3;
// Largest valid fork kind.
constexpr int kMaxForkKind = 2;

// One event as stored in a run file.
struct RunEvent
{
    int type = 0;
    bool takenRight = false;
    float duration = 1.0f;
    int kind = 0;
    int forkKind = 0;
    int forkId = -1;
    int loopId = -1;
    std::string label;
    std::string endLabel;
    std::string armLeft;
    std::string armRight;
    std::string name;

    bool operator==(const RunEvent& o) const
    {
        return type == o.type && takenRight == o.takenRight && duration == o.duration && kind == o.kind &&
               forkKind == o.forkKind && forkId == o.forkId && loopId == o.loopId && label == o.label &&
               endLabel == o.endLabel && armLeft == o.armLeft && armRight == o.armRight && name == o.name;
    }
};

// Escapes backslash, tab and line breaks so text fits one field.
inline std::string escape(const std::string& text)
{
    std::string out;
    for (char c : text) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '\t':
                out += "\\t";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            default:
                out += c;
        }
    }
    return out;
}

// Reverses escape.
inline std::string unescape(const std::string& text)
{
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            const char next = text[++i];
            switch (next) {
                case 't':
                    out += '\t';
                    break;
                case 'n':
                    out += '\n';
                    break;
                case 'r':
                    out += '\r';
                    break;
                default:
                    out += next;
            }
        } else {
            out += text[i];
        }
    }
    return out;
}

// Writes the header and one line per event.
inline void write(std::ostream& out, const std::vector<RunEvent>& events)
{
    out << "FVRUN " << kVersion << "\n";
    out << "events " << events.size() << "\n";
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    for (const RunEvent& e : events) {
        out << e.type << '\t' << (e.takenRight ? 1 : 0) << '\t' << e.duration << '\t' << e.kind << '\t'
            << e.forkKind << '\t' << e.forkId << '\t' << e.loopId << '\t' << escape(e.label) << '\t'
            << escape(e.endLabel) << '\t' << escape(e.armLeft) << '\t' << escape(e.armRight) << '\t'
            << escape(e.name) << '\n';
    }
}

// Reads a run file; on failure returns false and sets error.
inline bool read(std::istream& in, std::vector<RunEvent>& events, std::string& error)
{
    events.clear();
    error.clear();

    std::string line;
    if (!std::getline(in, line) || line.compare(0, 6, "FVRUN ") != 0) {
        error = "not a run file (missing FVRUN header)";
        return false;
    }
    const int version = std::atoi(line.c_str() + 6);
    if (version != kVersion) {
        error = "unsupported run file version " + line.substr(6) + " (this program reads version " +
                std::to_string(kVersion) + ")";
        return false;
    }

    if (!std::getline(in, line) || line.compare(0, 7, "events ") != 0) {
        error = "missing event count";
        return false;
    }
    const long expected = std::atol(line.c_str() + 7);
    if (expected < 0) {
        error = "bad event count";
        return false;
    }

    std::vector<RunEvent> result;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> fields;
        std::string field;
        std::istringstream split(line);
        while (std::getline(split, field, '\t')) {
            fields.push_back(field);
        }
        if (!line.empty() && line.back() == '\t') {
            fields.push_back("");
        }
        if (fields.size() != 12) {
            error = "line " + std::to_string(result.size() + 3) + ": expected 12 fields, found " +
                    std::to_string(fields.size());
            return false;
        }

        RunEvent e;
        e.type = std::atoi(fields[0].c_str());
        e.takenRight = fields[1] == "1";
        e.duration = static_cast<float>(std::atof(fields[2].c_str()));
        e.kind = std::atoi(fields[3].c_str());
        e.forkKind = std::atoi(fields[4].c_str());
        e.forkId = std::atoi(fields[5].c_str());
        e.loopId = std::atoi(fields[6].c_str());
        e.label = unescape(fields[7]);
        e.endLabel = unescape(fields[8]);
        e.armLeft = unescape(fields[9]);
        e.armRight = unescape(fields[10]);
        e.name = unescape(fields[11]);

        if (e.type < 0 || e.type > kMaxType || e.kind < 0 || e.kind > kMaxKind || e.forkKind < 0 ||
            e.forkKind > kMaxForkKind) {
            error = "line " + std::to_string(result.size() + 3) + ": value out of range";
            return false;
        }
        result.push_back(e);
    }

    if (static_cast<long>(result.size()) != expected) {
        error = "expected " + std::to_string(expected) + " events but found " +
                std::to_string(result.size()) + " (file cut off?)";
        return false;
    }
    events.swap(result);
    return true;
}

}
