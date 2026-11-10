// eventlog_test.cpp - Tests for the event log, the call stack worked out from it, run files on disk and file lookup.

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "CallStackTracker.h"
#include "EventLog.h"
#include "ProjectFolders.h"
#include "RunStore.h"
#include "test_util.h"

static FlowEvent makeEvent(FlowEventType type, const std::string& label = "", const std::string& name = "")
{
    FlowEvent e;
    e.type = type;
    e.label = label;
    e.name = name;
    return e;
}

static void conversionTests()
{
    FlowEvent e;
    e.type = FlowEventType::FORK_START;
    e.takenRight = true;
    e.duration = 2.5f;
    e.label = "x > 1";
    e.endLabel = "end";
    e.armLeft = "else";
    e.armRight = "case 2";
    e.kind = FlowKind::STATEMENT;
    e.forkKind = ForkKind::SWITCH;
    e.forkId = 7;
    e.loopId = 3;
    e.name = "main";

    const FlowEvent back = fromRunEvent(toRunEvent(e));
    CHECK(back.type == e.type);
    CHECK_EQ(back.takenRight, e.takenRight);
    CHECK_EQ(back.duration, e.duration);
    CHECK_EQ(back.label, e.label);
    CHECK_EQ(back.endLabel, e.endLabel);
    CHECK_EQ(back.armLeft, e.armLeft);
    CHECK_EQ(back.armRight, e.armRight);
    CHECK(back.kind == e.kind);
    CHECK(back.forkKind == e.forkKind);
    CHECK_EQ(back.forkId, e.forkId);
    CHECK_EQ(back.loopId, e.loopId);
    CHECK_EQ(back.name, e.name);

    CHECK_EQ(static_cast<int>(FlowEventType::FLOW_START), 0);
    CHECK_EQ(static_cast<int>(FlowEventType::FUNCTION_ENTER), 5);
    CHECK_EQ(static_cast<int>(FlowEventType::PROGRAM_END), 11);
}

static void logTests()
{
    BacklogGate gate;
    EventLog log;
    CHECK_EQ(log.size(), 0u);

    for (int i = 0; i < 5; ++i) {
        log.append(makeEvent(FlowEventType::FLOW_START, "e" + std::to_string(i)), gate);
    }
    CHECK_EQ(log.size(), 5u);
    CHECK_EQ(gate.outstanding(), 5);
    CHECK_EQ(log.readIndex(), 0u);

    std::size_t first = 99;
    std::deque<FlowEvent> taken = log.take(3, gate, first);
    CHECK_EQ(first, 0u);
    CHECK_EQ(taken.size(), 3u);
    CHECK_EQ(taken.front().label, std::string("e0"));
    CHECK_EQ(taken.back().label, std::string("e2"));
    CHECK_EQ(log.readIndex(), 3u);
    CHECK_EQ(gate.outstanding(), 2);

    taken = log.take(2, gate, first);
    CHECK_EQ(first, 3u);
    CHECK(taken.empty());
    CHECK_EQ(log.readIndex(), 3u);

    taken = log.take(static_cast<std::size_t>(-1), gate, first);
    CHECK_EQ(first, 3u);
    CHECK_EQ(taken.size(), 2u);
    CHECK_EQ(log.readIndex(), 5u);
    CHECK_EQ(gate.outstanding(), 0);

    log.rewind(gate);
    CHECK_EQ(log.readIndex(), 0u);
    CHECK_EQ(gate.outstanding(), 5);
    CHECK_EQ(log.size(), 5u);

    CHECK_EQ(log.snapshot().size(), 5u);

    std::vector<FlowEvent> other;
    other.push_back(makeEvent(FlowEventType::OUTPUT, "hello"));
    other.push_back(makeEvent(FlowEventType::PROGRAM_END));
    log.replace(other, gate);
    CHECK_EQ(log.size(), 2u);
    CHECK_EQ(log.readIndex(), 0u);
    CHECK_EQ(gate.outstanding(), 2);

    bool endsProgram = false;
    log.inspect([&](const std::vector<FlowEvent>& events) {
        endsProgram = events.back().type == FlowEventType::PROGRAM_END;
    });
    CHECK(endsProgram);
}

static void callStackTests()
{
    BacklogGate gate;
    EventLog log;
    log.append(makeEvent(FlowEventType::PROGRAM_START), gate);
    log.append(makeEvent(FlowEventType::FUNCTION_ENTER, "main()", "main"), gate);
    log.append(makeEvent(FlowEventType::FLOW_START, "stmt"), gate);
    log.append(makeEvent(FlowEventType::FUNCTION_ENTER, "f(x=1)", "f(x=1)"), gate);
    log.append(makeEvent(FlowEventType::FUNCTION_ENTER, "label only"), gate);
    log.append(makeEvent(FlowEventType::FUNCTION_EXIT), gate);
    log.append(makeEvent(FlowEventType::FUNCTION_EXIT), gate);
    log.append(makeEvent(FlowEventType::FUNCTION_EXIT), gate);
    log.append(makeEvent(FlowEventType::FUNCTION_EXIT), gate);

    CallStackTracker stack;
    CHECK(stack.frames().empty());

    stack.advanceTo(log, 1);
    CHECK(stack.frames().empty());

    stack.advanceTo(log, 2);
    CHECK_EQ(stack.frames().size(), 1u);
    CHECK_EQ(stack.frames()[0], std::string("main"));

    stack.advanceTo(log, 5);
    CHECK_EQ(stack.frames().size(), 3u);
    CHECK_EQ(stack.frames()[1], std::string("f(x=1)"));
    CHECK_EQ(stack.frames()[2], std::string("label only"));

    stack.advanceTo(log, 6);
    CHECK_EQ(stack.frames().size(), 2u);

    stack.advanceTo(log, 4);
    CHECK_EQ(stack.frames().size(), 2u);
    CHECK_EQ(stack.frames()[1], std::string("f(x=1)"));

    stack.advanceTo(log, 100);
    CHECK(stack.frames().empty());

    stack.reset();
    CHECK(stack.frames().empty());
    stack.advanceTo(log, 2);
    CHECK_EQ(stack.frames().size(), 1u);
}

static void storeTests()
{
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / "cv_eventlog_test";
    std::filesystem::create_directories(folder);
    const std::string file = (folder / "nested" / "run.fvrun").string();

    BacklogGate gate;
    EventLog empty;
    CHECK(!runstore::save(empty, file));

    EventLog log;
    FlowEvent a = makeEvent(FlowEventType::FUNCTION_ENTER, "tab\there", "main");
    a.duration = 0.25f;
    FlowEvent b = makeEvent(FlowEventType::FORK_START, "line\nbreak");
    b.takenRight = true;
    b.forkKind = ForkKind::DECISION;
    b.forkId = 4;
    log.append(a, gate);
    log.append(b, gate);
    log.append(makeEvent(FlowEventType::PROGRAM_END), gate);

    CHECK(runstore::save(log, file));

    std::vector<FlowEvent> loaded;
    CHECK(runstore::load(file, loaded));
    CHECK_EQ(loaded.size(), 3u);
    CHECK_EQ(loaded[0].label, std::string("tab\there"));
    CHECK_EQ(loaded[0].name, std::string("main"));
    CHECK_EQ(loaded[0].duration, 0.25f);
    CHECK_EQ(loaded[1].label, std::string("line\nbreak"));
    CHECK(loaded[1].takenRight);
    CHECK(loaded[1].forkKind == ForkKind::DECISION);
    CHECK_EQ(loaded[1].forkId, 4);
    CHECK(loaded[2].type == FlowEventType::PROGRAM_END);

    std::vector<FlowEvent> none;
    CHECK(!runstore::load((folder / "missing.fvrun").string(), none));

    std::error_code ignored;
    std::filesystem::remove_all(folder, ignored);
}

static void locateTests()
{
    const std::filesystem::path base = std::filesystem::temp_directory_path() / "cv_locate_test";
    std::error_code ignored;
    std::filesystem::remove_all(base, ignored);
    std::filesystem::create_directories(base / "first");
    std::filesystem::create_directories(base / "second");
    std::FILE* file = std::fopen((base / "second" / "x.cfg").string().c_str(), "w");
    CHECK(file != nullptr);
    if (file) {
        std::fclose(file);
    }

    const std::vector<std::filesystem::path> folders = {base / "first", base / "second"};
    CHECK_EQ(projectfolders::locate("x.cfg", folders), (base / "second" / "x.cfg").string());
    CHECK_EQ(projectfolders::locate("missing.cfg", folders), std::string("missing.cfg"));

    file = std::fopen((base / "first" / "x.cfg").string().c_str(), "w");
    if (file) {
        std::fclose(file);
    }
    CHECK_EQ(projectfolders::locate("x.cfg", folders), (base / "first" / "x.cfg").string());

    std::filesystem::remove_all(base, ignored);
}

int main()
{
    locateTests();
    conversionTests();
    logTests();
    callStackTests();
    storeTests();
    return finish("eventlog_test");
}
