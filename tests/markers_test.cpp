// markers_test.cpp - Tests for the markers, the stop request and LiveSession.

#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <string>
#include <vector>

#include "LiveSession.h"
#include "Markers.h"
#include "test_util.h"

class Recorder : public TrackerHandler
{
public:
    std::vector<std::string> events;
    int stops = 0;

    void onFunctionEnter(const std::string& n) override { events.push_back("enter " + n); }

    void onFunctionExit(const std::string& n) override { events.push_back("exit " + n); }

    void onReturn(const std::string& v) override { events.push_back("return " + v); }

    void onBranchEnter(bool c) override { events.push_back(c ? "branch true" : "branch false"); }

    void onBranchExit(bool) override { events.push_back("end branch"); }

    void onDecision(const std::string& t, bool v) override
    {
        events.push_back("decide " + t + (v ? " yes" : " no"));
    }

    void onSwitchEnter(const std::string& e, const std::string& v) override
    {
        events.push_back("switch " + e + " = " + v);
    }

    void onSwitchExit() override { events.push_back("end switch"); }

    void onLoopEnter(const std::string& t) override { events.push_back("loop " + t); }

    void onLoopExit(const std::string& t) override { events.push_back("end " + t); }

    void onLoopIteration(const std::string& t, int i) override
    {
        events.push_back(t + " #" + std::to_string(i));
    }

    void onBlockEnter(const std::string& n) override { events.push_back("block " + n); }

    void onBlockExit(const std::string& n) override { events.push_back("end block " + n); }

    void onStatement(const std::string& t) override { events.push_back("stmt " + t); }

    void onInput(const std::string& t) override { events.push_back("in " + t); }

    void onInputDone(const std::string&) override { events.push_back("in done"); }

    void onOutput(const std::string& t) override { events.push_back("out " + t); }

    void onProgramStart() override { events.push_back("start"); }

    void onProgramEnd() override { events.push_back("end"); }

    void onStop() override { ++stops; }
};

using Events = std::vector<std::string>;

static std::shared_ptr<Recorder> fresh()
{
    Trackers::clearHandlers();
    Trackers::resetStop();
    auto rec = std::make_shared<Recorder>();
    Trackers::addHandler(rec);
    return rec;
}

static int twice(int x)
{
    FV_FUNCTION(x);
    return FV_RETURN(x * 2);
}

static void valueTextTests()
{
    CHECK_EQ(fv::valueText(42), std::string("42"));
    CHECK_EQ(fv::valueText(true), std::string("true"));
    CHECK_EQ(fv::valueText(false), std::string("false"));
    CHECK_EQ(fv::valueText('x'), std::string("'x'"));
    CHECK_EQ(fv::valueText(std::string("hi")), std::string("\"hi\""));
    CHECK_EQ(fv::valueText("hi"), std::string("\"hi\""));
    CHECK_EQ(fv::valueText(2.5), std::string("2.5"));

    struct NoPrint
    {};

    CHECK_EQ(fv::valueText(NoPrint{}), std::string("?"));

    const std::string longText = fv::valueText(std::string(60, 'a'));
    CHECK_EQ(longText.size(), 24u);
    CHECK_EQ(longText.substr(21), std::string("..."));
}

static void argumentTextTests()
{
    int a = 3;
    std::string s = "ab";
    CHECK_EQ(fv::argsOf("a")(a), std::string("a=3"));
    CHECK_EQ(fv::argsOf("a, s")(a, s), std::string("a=3, s=\"ab\""));

    CHECK_EQ(fv::argsOf("f(a, 1), a")(7, a), std::string("f(a, 1)=7, a=3"));
    CHECK_EQ(fv::argsOf("")(), std::string(""));
}

static void cleanSourceTests()
{
    CHECK_EQ(fv::cleanSource("FV_COND(x > 1)"), std::string("x > 1"));
    CHECK_EQ(fv::cleanSource("FV_COND(a) && FV_COND(b)"), std::string("a && b"));
    CHECK_EQ(fv::cleanSource("FV_COND(f(x)) || y"), std::string("f(x) || y"));
    CHECK_EQ(fv::cleanSource("FV_INPUT(in >> n)"), std::string("in >> n"));
    CHECK_EQ(fv::cleanSource("FV_OUTPUT(std::cout << 1)"), std::string("std::cout << 1"));
    CHECK_EQ(fv::cleanSource("plain(text)"), std::string("plain(text)"));

    CHECK_EQ(fv::cleanSource("MY_FV_COND(x)"), std::string("MY_FV_COND(x)"));
    CHECK_EQ(fv::cleanSource(""), std::string(""));
}

static void functionTests()
{
    auto rec = fresh();
    const int r = twice(4);
    CHECK_EQ(r, 8);
    const Events expected = {"enter twice(x=4)", "return 8", "exit twice"};
    CHECK(rec->events == expected);
}

static void branchAndDecisionTests()
{
    auto rec = fresh();
    int x = 5;
    if (FV_COND(x > 3)) {
        BRANCH_SCOPE(true);
    }
    if (FV_COND(x > 9)) {
        BRANCH_SCOPE(true);
    }
    const Events expected = {"decide x > 3 yes", "branch true", "end branch", "decide x > 9 no"};
    CHECK(rec->events == expected);
}

static void switchTests()
{
    auto rec = fresh();
    int k = 2;
    FV_SWITCH(k % 3)
    {
        case 2:
            break;
        default:
            break;
    }
    const Events expected = {"switch k % 3 = 2", "end switch"};
    CHECK(rec->events == expected);
}

static void loopTests()
{
    auto rec = fresh();
    {
        LOOP_SCOPE("for");
        for (int i = 0; i < 2; ++i) {
            FV_ITERATION();
        }
        {
            LOOP_SCOPE("while");
            FV_ITERATION();
            FV_ITERATION();
            FV_ITERATION();
        }
        FV_ITERATION();
    }
    FV_ITERATION();

    const Events expected = {"loop for", "for #1",   "for #2",    "loop while", "while #1",
                             "while #2", "while #3", "end while", "for #3",     "end for"};
    CHECK(rec->events == expected);
}

static void blockAndIoTests()
{
    auto rec = fresh();
    BLOCK_ENTER("b");
    BLOCK_EXIT("b");
    int n = 0;
    FV_STMT(n = 1);
    FV_OUTPUT(n = 2);
    const Events expected = {"block b", "end block b", "stmt n = 1", "out n = 2"};
    CHECK(rec->events == expected);
}

static void programScopeTests()
{
    auto rec = fresh();
    {
        ProgramScope program;
        Trackers::trackStatement("x");
    }
    const Events expected = {"start", "stmt x", "end"};
    CHECK(rec->events == expected);
}

static void handlerTests()
{
    auto first = fresh();
    auto second = std::make_shared<Recorder>();
    Trackers::addHandler(second);
    Trackers::addHandler(nullptr);
    Trackers::trackStatement("a");
    CHECK_EQ(first->events.size(), 1u);
    CHECK_EQ(second->events.size(), 1u);

    Trackers::clearHandlers();
    Trackers::trackStatement("b");
    CHECK_EQ(first->events.size(), 1u);
    CHECK_EQ(second->events.size(), 1u);
}

static void stopTests()
{
    auto rec = fresh();
    CHECK(!Trackers::stopRequested());

    Trackers::requestStop();
    CHECK(Trackers::stopRequested());
    CHECK_EQ(rec->stops, 1);

    bool thrown = false;
    try {
        Trackers::trackStatement("never");
    } catch (const StopRequested&) {
        thrown = true;
    }
    CHECK(thrown);
    CHECK(rec->events.empty());

    bool exitThrew = false;
    try {
        Trackers::trackFunctionExit("f");
        Trackers::trackLoopExit("for");
        Trackers::trackProgramEnd();
    } catch (...) {
        exitThrew = true;
    }
    CHECK(!exitThrew);

    Trackers::resetStop();
    CHECK(!Trackers::stopRequested());
    Trackers::trackStatement("again");
    CHECK_EQ(rec->events.back(), std::string("stmt again"));
}

static void throwingHandlerTests()
{
    struct Bad : TrackerHandler
    {
        void onFunctionExit(const std::string&) override { throw 1; }
    };

    Trackers::clearHandlers();
    Trackers::resetStop();
    auto good = std::make_shared<Recorder>();
    Trackers::addHandler(std::make_shared<Bad>());
    Trackers::addHandler(good);

    bool escaped = false;
    try {
        Trackers::trackFunctionExit("f");
    } catch (...) {
        escaped = true;
    }
    CHECK(!escaped);
    CHECK_EQ(good->events.size(), 1u);
}

static void waitUntilDone(LiveSession& session)
{
    for (int i = 0; i < 5000 && session.outcome() == LiveSession::Running; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

static void sessionTests()
{
    {
        fresh();
        LiveSession session([] { Trackers::trackStatement("hi"); });
        CHECK_EQ(session.outcome(), LiveSession::NotStarted);
        session.start();
        waitUntilDone(session);
        session.stopAndJoin();
        CHECK_EQ(session.outcome(), LiveSession::Finished);
    }
    {
        auto rec = fresh();
        LiveSession session([] { Trackers::trackStatement("hi"); });
        session.start();
        waitUntilDone(session);
        session.stopAndJoin();
        const Events expected = {"start", "stmt hi", "end"};
        CHECK(rec->events == expected);
    }
    {
        fresh();
        LiveSession session([] { throw std::runtime_error("boom"); });
        session.start();
        waitUntilDone(session);
        session.stopAndJoin();
        CHECK_EQ(session.outcome(), LiveSession::Failed);
    }
    {
        fresh();
        LiveSession session([] { throw 7; });
        session.start();
        waitUntilDone(session);
        session.stopAndJoin();
        CHECK_EQ(session.outcome(), LiveSession::Failed);
    }
    {
        fresh();
        LiveSession session([] {
            for (;;) {
                Trackers::trackStatement("spin");
            }
        });
        session.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        session.stopAndJoin();
        CHECK_EQ(session.outcome(), LiveSession::Stopped);
    }
    Trackers::resetStop();
}

struct Chatty
{
    int v;
};

static std::ostream& operator<<(std::ostream& os, const Chatty& c)
{
    Trackers::trackStatement("printing a Chatty");
    return os << "chatty " << c.v << " " << fv::valueText(c);
}

static void streamedValueTests()
{
    auto recorder = std::make_shared<Recorder>();
    Trackers::clearHandlers();
    Trackers::addHandler(recorder);

    const std::string text = fv::valueText(Chatty{7});
    CHECK_EQ(text, "chatty 7 ?");
    CHECK(recorder->events.empty());
    CHECK(!fv::describing());

    Trackers::trackStatement("after");
    CHECK_EQ(recorder->events.size(), static_cast<std::size_t>(1));

    Trackers::clearHandlers();
}

int main()
{
    valueTextTests();
    argumentTextTests();
    streamedValueTests();
    cleanSourceTests();
    functionTests();
    branchAndDecisionTests();
    switchTests();
    loopTests();
    blockAndIoTests();
    programScopeTests();
    handlerTests();
    stopTests();
    throwingHandlerTests();
    sessionTests();
    Trackers::clearHandlers();
    return finish("markers_test");
}
