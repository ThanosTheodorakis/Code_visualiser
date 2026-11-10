// runfile_test.cpp - Tests for saving and loading a recorded run.

#include <sstream>
#include <string>
#include <vector>

#include "RunFile.h"
#include "test_util.h"

using runfile::RunEvent;

static RunEvent makeEvent(int type, const std::string& label = "", const std::string& name = "")
{
    RunEvent e;
    e.type = type;
    e.label = label;
    e.name = name;
    return e;
}

static std::string toText(const std::vector<RunEvent>& events)
{
    std::ostringstream out;
    runfile::write(out, events);
    return out.str();
}

static bool fromText(const std::string& text, std::vector<RunEvent>& events, std::string& error)
{
    std::istringstream in(text);
    return runfile::read(in, events, error);
}

static void escapeTests()
{
    const std::string tricky = "a\tb\nc\rd\\e\\\\ \\t end";
    CHECK_EQ(runfile::unescape(runfile::escape(tricky)), tricky);
    CHECK_EQ(runfile::escape("a\tb"), std::string("a\\tb"));
    CHECK_EQ(runfile::escape("plain text"), std::string("plain text"));
    CHECK_EQ(runfile::escape("").size(), 0u);
    CHECK(runfile::escape(tricky).find('\t') == std::string::npos);
    CHECK(runfile::escape(tricky).find('\n') == std::string::npos);
}

static void roundTripTests()
{
    std::vector<RunEvent> events;

    RunEvent fork = makeEvent(3, "if x > 3", "");
    fork.takenRight = true;
    fork.duration = 2.0f;
    fork.forkKind = 2;
    fork.forkId = 7;
    fork.armLeft = "else";
    fork.armRight = "x > 3";
    events.push_back(fork);

    RunEvent loop = makeEvent(1, "for");
    loop.loopId = 4;
    loop.duration = 2.2f;
    events.push_back(loop);

    RunEvent odd = makeEvent(5, "function enter: f(a=\"x\ty\")", "f(a=\"x\ty\")");
    odd.endLabel = "line1\nline2";
    odd.kind = 3;
    odd.duration = 0.1f;
    events.push_back(odd);

    events.push_back(makeEvent(10));
    events.push_back(makeEvent(11, "", "last"));

    std::vector<RunEvent> back;
    std::string error;
    CHECK(fromText(toText(events), back, error));
    CHECK(error.empty());
    CHECK_EQ(back.size(), events.size());
    CHECK(back == events);

    std::vector<RunEvent> none;
    CHECK(fromText(toText(none), back, error));
    CHECK(back.empty());
}

static void formatTests()
{
    std::vector<RunEvent> events{makeEvent(0, "a"), makeEvent(10)};
    const std::string text = toText(events);
    CHECK(text.compare(0, 8, "FVRUN 1\n") == 0);
    CHECK(text.find("events 2\n") != std::string::npos);

    int lines = 0;
    for (char c : text) {
        if (c == '\n') {
            ++lines;
        }
    }
    CHECK_EQ(lines, 4);

    std::string crlf;
    for (char c : text) {
        if (c == '\n') {
            crlf += '\r';
        }
        crlf += c;
    }
    std::vector<RunEvent> back;
    std::string error;
    CHECK(fromText(crlf, back, error));
    CHECK(back == events);
}

static void badInputTests()
{
    std::vector<RunEvent> out{makeEvent(1)};
    std::string error;

    CHECK(!fromText("", out, error));
    CHECK(out.empty());
    CHECK(!error.empty());

    CHECK(!fromText("hello world\n", out, error));
    CHECK(error.find("not a run file") != std::string::npos);

    CHECK(!fromText("FVRUN 99\nevents 0\n", out, error));
    CHECK(error.find("version") != std::string::npos);

    CHECK(!fromText("FVRUN 1\n", out, error));

    CHECK(!fromText("FVRUN 1\nevents -3\n", out, error));

    std::vector<RunEvent> two{makeEvent(0), makeEvent(10)};
    std::string text = toText(two);
    text = text.substr(0, text.rfind("10\t"));
    CHECK(!fromText(text, out, error));
    CHECK(error.find("cut off") != std::string::npos);

    CHECK(!fromText("FVRUN 1\nevents 1\n1\t2\t3\n", out, error));
    CHECK(error.find("12 fields") != std::string::npos);

    CHECK(!fromText("FVRUN 1\nevents 1\n99\t0\t1\t0\t0\t-1\t-1\t\t\t\t\t\n", out, error));
    CHECK(error.find("out of range") != std::string::npos);
    CHECK(!fromText("FVRUN 1\nevents 1\n0\t0\t1\t9\t0\t-1\t-1\t\t\t\t\t\n", out, error));
    CHECK(!fromText("FVRUN 1\nevents 1\n0\t0\t1\t0\t9\t-1\t-1\t\t\t\t\t\n", out, error));
}

int main()
{
    escapeTests();
    roundTripTests();
    formatTests();
    badInputTests();
    return finish("runfile_test");
}
