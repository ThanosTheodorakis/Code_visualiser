// Markers.cpp - Implementation of the markers and the Trackers registry.

#include "Markers.h"

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

namespace {
using HandlerList = std::vector<std::shared_ptr<TrackerHandler>>;

struct State
{
    std::mutex handlerMutex;
    std::shared_ptr<const HandlerList> handlers = std::make_shared<const HandlerList>();
    std::atomic<bool> stop{false};
};

State& state()
{
    static State instance;
    return instance;
}

std::shared_ptr<const HandlerList> snapshot()
{
    std::lock_guard<std::mutex> lock(state().handlerMutex);
    return state().handlers;
}

template <class F> void dispatchEnter(F call)
{
    if (fv::describing()) {
        return;
    }
    if (state().stop.load()) {
        throw StopRequested();
    }
    const auto handlers = snapshot();
    for (const auto& handler : *handlers) {
        call(*handler);
    }
}

template <class F> void dispatchExit(F call) noexcept
{
    if (fv::describing()) {
        return;
    }
    try {
        const auto handlers = snapshot();
        for (const auto& handler : *handlers) {
            try {
                call(*handler);
            } catch (...) {}
        }
    } catch (...) {}
}

class ConsoleHandler : public TrackerHandler
{
public:
    void onFunctionEnter(const std::string& n) override { std::cout << "FunctionEnter: " << n << std::endl; }

    void onFunctionExit(const std::string& n) override { std::cout << "FunctionExit: " << n << std::endl; }

    void onReturn(const std::string& v) override { std::cout << "Return: " << v << std::endl; }

    void onBranchEnter(bool c) override { std::cout << "BranchEnter: " << std::boolalpha << c << std::endl; }

    void onBranchExit(bool c) override { std::cout << "BranchExit: " << std::boolalpha << c << std::endl; }

    void onDecision(const std::string& t, bool v) override
    {
        std::cout << "Decision: " << t << " = " << std::boolalpha << v << std::endl;
    }

    void onSwitchEnter(const std::string& e, const std::string& v) override
    {
        std::cout << "SwitchEnter: " << e << " = " << v << std::endl;
    }

    void onSwitchExit() override { std::cout << "SwitchExit" << std::endl; }

    void onLoopEnter(const std::string& t) override { std::cout << "loopEnter: " << t << std::endl; }

    void onLoopExit(const std::string& t) override { std::cout << "loopExit: " << t << std::endl; }

    void onBlockEnter(const std::string& n) override { std::cout << "BlockEnter: " << n << std::endl; }

    void onBlockExit(const std::string& n) override { std::cout << "BlockExit: " << n << std::endl; }

    void onLoopIteration(const std::string& t, int i) override
    {
        std::cout << "loopIteration: " << t << " " << i << std::endl;
    }

    void onStatement(const std::string& t) override { std::cout << "Statement: " << t << std::endl; }

    void onProgramStart() override { std::cout << "ProgramStart" << std::endl; }

    void onProgramEnd() override { std::cout << "ProgramEnd" << std::endl; }

    void onInput(const std::string& t) override { std::cout << "Input: " << t << std::endl; }

    void onInputDone(const std::string& t) override { std::cout << "InputDone: " << t << std::endl; }

    void onOutput(const std::string& t) override { std::cout << "Output: " << t << std::endl; }
};

struct LoopState
{
    std::string type;
    int iterations;
};

std::vector<LoopState>& loops()
{
    static thread_local std::vector<LoopState> instance;
    return instance;
}
}

bool Trackers::stopRequested()
{
    return state().stop.load();
}

void Trackers::requestStop()
{
    state().stop.store(true);

    const auto handlers = snapshot();
    for (const auto& handler : *handlers) {
        try {
            handler->onStop();
        } catch (...) {}
    }
}

void Trackers::resetStop()
{
    state().stop.store(false);
}

void Trackers::addHandler(std::shared_ptr<TrackerHandler> handler)
{
    if (!handler) {
        return;
    }
    std::lock_guard<std::mutex> lock(state().handlerMutex);
    auto next = std::make_shared<HandlerList>(*state().handlers);
    next->push_back(std::move(handler));
    state().handlers = std::move(next);
}

void Trackers::clearHandlers()
{
    std::lock_guard<std::mutex> lock(state().handlerMutex);
    state().handlers = std::make_shared<const HandlerList>();
}

void Trackers::installConsoleHandler()
{
    addHandler(std::make_shared<ConsoleHandler>());
}

void Trackers::trackFunctionEnter(const std::string& n)
{
    dispatchEnter([&](TrackerHandler& h) { h.onFunctionEnter(n); });
}

void Trackers::trackFunctionExit(const std::string& n)
{
    dispatchExit([&](TrackerHandler& h) { h.onFunctionExit(n); });
}

void Trackers::trackReturn(const std::string& v)
{
    dispatchEnter([&](TrackerHandler& h) { h.onReturn(v); });
}

void Trackers::trackBranchEnter(bool c, const std::string& text)
{
    dispatchEnter([&](TrackerHandler& h) { h.onBranchEnter(c, text); });
}

void Trackers::trackBranchExit(bool c)
{
    dispatchExit([&](TrackerHandler& h) { h.onBranchExit(c); });
}

void Trackers::trackDecision(const std::string& text, bool value)
{
    dispatchEnter([&](TrackerHandler& h) { h.onDecision(text, value); });
}

void Trackers::trackSwitchEnter(const std::string& e, const std::string& v)
{
    dispatchEnter([&](TrackerHandler& h) { h.onSwitchEnter(e, v); });
}

void Trackers::trackSwitchExit()
{
    dispatchExit([&](TrackerHandler& h) { h.onSwitchExit(); });
}

void Trackers::trackLoopEnter(const std::string& t)
{
    dispatchEnter([&](TrackerHandler& h) { h.onLoopEnter(t); });
    loops().push_back({t, 0});
}

void Trackers::trackLoopExit(const std::string& t)
{
    dispatchExit([&](TrackerHandler& h) { h.onLoopExit(t); });
    if (!loops().empty()) {
        loops().pop_back();
    }
}

void Trackers::trackLoopIteration()
{
    if (loops().empty()) {
        return;
    }
    const std::string type = loops().back().type;
    const int index = loops().back().iterations + 1;
    dispatchEnter([&](TrackerHandler& h) { h.onLoopIteration(type, index); });
    loops().back().iterations = index;
}

void Trackers::trackStatement(const std::string& text)
{
    dispatchEnter([&](TrackerHandler& h) { h.onStatement(text); });
}

void Trackers::trackProgramStart()
{
    dispatchEnter([&](TrackerHandler& h) { h.onProgramStart(); });
}

void Trackers::trackProgramEnd()
{
    dispatchExit([&](TrackerHandler& h) { h.onProgramEnd(); });
}

void Trackers::trackInput(const std::string& text)
{
    dispatchEnter([&](TrackerHandler& h) { h.onInput(text); });
}

void Trackers::trackInputDone(const std::string& text)
{
    dispatchExit([&](TrackerHandler& h) { h.onInputDone(text); });
}

void Trackers::trackOutput(const std::string& text)
{
    dispatchEnter([&](TrackerHandler& h) { h.onOutput(text); });
}

void Trackers::trackBlockEnter(const std::string& n)
{
    dispatchEnter([&](TrackerHandler& h) { h.onBlockEnter(n); });
}

void Trackers::trackBlockExit(const std::string& n)
{
    dispatchExit([&](TrackerHandler& h) { h.onBlockExit(n); });
}
