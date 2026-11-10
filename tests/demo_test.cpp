// demo_test.cpp - Test that the demo program reports the expected events.

#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "DemoProgram.h"

class Recorder : public TrackerHandler
{
public:
    std::vector<std::string> events;

    void onFunctionEnter(const std::string& n) override { events.push_back("enter " + n); }

    void onFunctionExit(const std::string& n) override { events.push_back("exit  " + n); }

    void onReturn(const std::string& v) override { events.push_back("return " + v); }

    void onBranchEnter(bool c) override { events.push_back(c ? "branch true" : "branch false"); }

    void onBranchExit(bool) override { events.push_back("end branch"); }

    void onDecision(const std::string& t, bool v) override
    {
        events.push_back(std::string("decide ") + t + (v ? " yes" : " no"));
    }

    void onSwitchEnter(const std::string& e, const std::string& v) override
    {
        events.push_back("switch " + e + " = " + v);
    }

    void onSwitchExit() override { events.push_back("end switch"); }

    void onLoopEnter(const std::string& t) override { events.push_back("loop " + t); }

    void onLoopExit(const std::string&) override { events.push_back("end loop"); }

    void onLoopIteration(const std::string&, int i) override
    {
        events.push_back("iteration " + std::to_string(i));
    }

    void onStatement(const std::string& t) override { events.push_back("statement " + t); }

    void onProgramStart() override { events.push_back("program start"); }

    void onProgramEnd() override { events.push_back("program end"); }

    void onInput(const std::string& t) override { events.push_back("input " + t); }

    void onInputDone(const std::string&) override { events.push_back("input done"); }

    void onOutput(const std::string& t) override { events.push_back("output " + t); }
};

int main()
{
    auto rec = std::make_shared<Recorder>();
    Trackers::addHandler(rec);

    std::istringstream keyboard("3\n");
    int result = 0;
    {
        ProgramScope program;
        result = demo::runDemo(keyboard);
    }

    const std::string add = "statement total += square(i)";
    const std::string weigh = "statement weighted += weight(i)";
    const std::vector<std::string> expected = {
        "program start",
        "enter runDemo",
        "enter readCount",
        "output std::cout << \"how many squares (1-20)? \" << std::flush",
        "input in >> n",
        "input done",
        "return 3",
        "exit  readCount",
        "enter sumOfSquares(n=3)",
        "loop for",
        "iteration 1",
        add,
        "enter square(x=1)",
        "return 1",
        "exit  square",
        weigh,
        "enter weight(i=1)",
        "switch i % 3 = 1",
        "end switch",
        "return 1",
        "exit  weight",
        "branch false",
        "end branch",
        "iteration 2",
        add,
        "enter square(x=2)",
        "return 4",
        "exit  square",
        weigh,
        "enter weight(i=2)",
        "switch i % 3 = 2",
        "end switch",
        "return 2",
        "exit  weight",
        "branch true",
        "statement ++evenSteps",
        "end branch",
        "iteration 3",
        add,
        "enter square(x=3)",
        "return 9",
        "exit  square",
        weigh,
        "enter weight(i=3)",
        "switch i % 3 = 0",
        "end switch",
        "return 3",
        "exit  weight",
        "branch false",
        "end branch",
        "end loop",
        "loop do-while",
        "iteration 1",
        "statement ++rounds",
        "iteration 2",
        "statement ++rounds",
        "end loop",
        "output std::cout << \"  even steps: \" << evenSteps << std::endl",
        "return 14",
        "exit  sumOfSquares",
        "decide total > 10 yes",
        "decide count > 1 yes",
        "decide total > 100 no",
        "statement size = bigAndMany ? size : \"small\"",
        "branch true",
        "output std::cout << \"big: \" << total << std::endl",
        "end branch",
        "return 14",
        "exit  runDemo",
        "program end"};

    int failures = 0;
    if (result != 14) {
        std::printf("FAIL: runDemo() returned %d, expected 14\n", result);
        ++failures;
    }
    if (rec->events != expected) {
        std::printf("FAIL: events differ\n  got %zu events, expected %zu\n  got:", rec->events.size(),
                    expected.size());
        for (const auto& e : rec->events) {
            std::printf(" [%s]", e.c_str());
        }
        std::printf("\n");
        ++failures;
    }

    std::printf("demo_test: %s\n", failures == 0 ? "OK" : "FAILED");
    return failures == 0 ? 0 : 1;
}
