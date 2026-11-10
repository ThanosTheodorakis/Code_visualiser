// RendererHandler.cpp - Implementation of RendererHandler.

#include "RendererHandler.h"

#include <atomic>
#include <string>
#include <iostream>
#include <vector>

#include "Renderer.h"

namespace {
std::atomic<int> g_nextForkId{1};
std::atomic<int> g_nextLoopId{1};
thread_local std::vector<int> g_branchIdStack;
thread_local std::vector<int> g_loopIdStack;

thread_local bool g_collapsing = false;
thread_local int g_hiddenNesting = 0;
thread_local int g_hiddenIterations = 0;
thread_local std::string g_pendingReturn;

bool swallowEnter()
{
    if (!g_collapsing) {
        return false;
    }
    ++g_hiddenNesting;
    return true;
}

bool swallowExit()
{
    if (!g_collapsing || g_hiddenNesting == 0) {
        return false;
    }
    --g_hiddenNesting;
    return true;
}

int popId(std::vector<int>& stack, const char* what)
{
    if (stack.empty()) {
        std::cerr << "[RendererHandler] warning: " << what << " exit without matching enter\n";
        return -1;
    }
    const int id = stack.back();
    stack.pop_back();
    return id;
}
}

void RendererHandler::onProgramStart()
{
    renderer_.waitForCapacity();
    renderer_.enqueueProgramStartEvent();
    renderer_.enqueueFlowEvent(0.6f);
}

void RendererHandler::onProgramEnd()
{
    renderer_.waitForCapacity();
    renderer_.enqueueProgramEndEvent();
}

namespace {
std::string shortLabel(const std::string& text);
}

void RendererHandler::onFunctionEnter(const std::string& name)
{
    if (swallowEnter()) {
        return;
    }
    renderer_.waitForCapacity();
    renderer_.enqueueFunctionEnter("function enter: " + name, name);
    renderer_.enqueueFlowEvent(1.2f);
}

void RendererHandler::onReturn(const std::string& value)
{
    if (!g_collapsing) {
        g_pendingReturn = value;
    }
}

void RendererHandler::onFunctionExit(const std::string& name)
{
    if (swallowExit()) {
        g_pendingReturn.clear();
        return;
    }
    renderer_.waitForCapacity();
    std::string label = "function exit: " + name;
    if (!g_pendingReturn.empty()) {
        label += " -> " + g_pendingReturn;
        g_pendingReturn.clear();
    }
    renderer_.enqueueFunctionExit(label);
    renderer_.enqueueFlowEvent(0.9f);
}

void RendererHandler::onBranchEnter(bool condition, const std::string& text)
{
    if (swallowEnter()) {
        return;
    }
    renderer_.waitForCapacity();
    const int id = g_nextForkId.fetch_add(1);
    g_branchIdStack.push_back(id);
    if (text.empty()) {
        renderer_.enqueueForkStartEvent(condition, id, 2.2f, "branch enter");
    } else {
        renderer_.enqueueForkStartEvent(condition, id, 2.2f, shortLabel("if " + text), "else",
                                        shortLabel(text));
    }
    renderer_.enqueueFlowEvent(1.2f);
}

void RendererHandler::onBranchExit(bool)
{
    if (swallowExit()) {
        return;
    }
    renderer_.waitForCapacity();
    renderer_.enqueueForkEndEvent(popId(g_branchIdStack, "branch"), 0.9f, "if exit");
}

void RendererHandler::onDecision(const std::string& text, bool value)
{
    if (g_collapsing) {
        return;
    }
    renderer_.waitForCapacity();
    const int id = g_nextForkId.fetch_add(1);
    renderer_.enqueueForkStartEvent(value, id, 2.0f, shortLabel(text), "", "", Renderer::ForkKind::DECISION);
    renderer_.enqueueFlowEvent(0.6f);
    renderer_.enqueueForkEndEvent(id, 0.9f, "");
}

void RendererHandler::onSwitchEnter(const std::string& expression, const std::string& value)
{
    if (swallowEnter()) {
        return;
    }
    renderer_.waitForCapacity();
    const int id = g_nextForkId.fetch_add(1);
    g_branchIdStack.push_back(id);
    renderer_.enqueueForkStartEvent(true, id, 2.2f, shortLabel("switch " + expression + " = " + value),
                                    "default", shortLabel("case " + value), Renderer::ForkKind::SWITCH);
    renderer_.enqueueFlowEvent(1.2f);
}

void RendererHandler::onSwitchExit()
{
    if (swallowExit()) {
        return;
    }
    renderer_.waitForCapacity();
    renderer_.enqueueForkEndEvent(popId(g_branchIdStack, "switch"), 0.9f, "switch exit");
}

void RendererHandler::onLoopEnter(const std::string& loopType)
{
    if (swallowEnter()) {
        return;
    }
    renderer_.waitForCapacity();
    const int id = g_nextLoopId.fetch_add(1);
    g_loopIdStack.push_back(id);
    renderer_.enqueueLoopEvent(3.5f, "loop enter: " + loopType, id);
    renderer_.enqueueFlowEvent(1.2f);
}

void RendererHandler::onLoopExit(const std::string& loopType)
{
    if (swallowExit()) {
        return;
    }
    renderer_.waitForCapacity();

    if (g_collapsing) {
        g_collapsing = false;
        g_hiddenNesting = 0;
        renderer_.enqueueFlowEvent(1.0f,
                                   std::to_string(g_hiddenIterations) + (g_hiddenIterations == 1
                                                                             ? " more iteration hidden"
                                                                             : " more iterations hidden"),
                                   "", Renderer::FlowKind::ITERATION);
        g_hiddenIterations = 0;
    }

    renderer_.enqueueLoopEndEvent(popId(g_loopIdStack, "loop"), 1.0f, "loop exit: " + loopType);
    renderer_.enqueueFlowEvent(1.0f);
}

void RendererHandler::onBlockEnter(const std::string& name)
{
    if (swallowEnter()) {
        return;
    }
    renderer_.waitForCapacity();
    renderer_.enqueueFlowEvent(1.0f, "block enter: " + name, "", Renderer::FlowKind::BLOCK);
}

void RendererHandler::onBlockExit(const std::string& name)
{
    if (swallowExit()) {
        return;
    }
    renderer_.waitForCapacity();
    renderer_.enqueueFlowEvent(1.0f, "", "block exit: " + name, Renderer::FlowKind::BLOCK);
}

void RendererHandler::onLoopIteration(const std::string&, int index)
{
    if (g_collapsing) {
        if (g_hiddenNesting == 0) {
            ++g_hiddenIterations;
        }
        return;
    }

    const int limit = renderer_.maxIterationsShown();
    if (limit > 0 && index > limit) {
        g_collapsing = true;
        g_hiddenNesting = 0;
        g_hiddenIterations = 1;
        return;
    }

    renderer_.waitForCapacity();
    renderer_.enqueueFlowEvent(1.0f, "iteration " + std::to_string(index), "", Renderer::FlowKind::ITERATION);
}

void RendererHandler::onStatement(const std::string& text)
{
    const std::size_t kMaxChars = 30;
    const std::string shown = text.size() > kMaxChars ? text.substr(0, kMaxChars - 3) + "..." : text;

    if (g_collapsing) {
        return;
    }

    renderer_.waitForCapacity();
    renderer_.enqueueFlowEvent(1.0f, "statement: " + shown, "", Renderer::FlowKind::STATEMENT);
}

namespace {
std::string dropStream(const std::string& text, const char* const* prefixes, int count)
{
    for (int i = 0; i < count; ++i) {
        const std::string prefix = prefixes[i];
        if (text.compare(0, prefix.size(), prefix) == 0) {
            return text.substr(prefix.size());
        }
    }
    return text;
}

std::string shortLabel(const std::string& text)
{
    const std::size_t kMaxChars = 30;
    return text.size() > kMaxChars ? text.substr(0, kMaxChars - 3) + "..." : text;
}
}

void RendererHandler::onInput(const std::string& text)
{
    if (g_collapsing) {
        ++g_hiddenNesting;
        return;
    }
    static const char* const kStreams[] = {"std::cin >> ", "cin >> "};
    renderer_.waitForCapacity();
    renderer_.enqueueInputEvent("input: " + shortLabel(dropStream(text, kStreams, 2)));
}

void RendererHandler::onInputDone(const std::string&)
{
    if (swallowExit()) {
        return;
    }

    renderer_.enqueueInputDoneEvent();
    renderer_.enqueueFlowEvent(0.6f);
}

void RendererHandler::onOutput(const std::string& text)
{
    if (g_collapsing) {
        return;
    }
    static const char* const kStreams[] = {"std::cout << ", "cout << "};
    renderer_.waitForCapacity();
    renderer_.enqueueOutputEvent("output: " + shortLabel(dropStream(text, kStreams, 2)));
    renderer_.enqueueFlowEvent(0.6f);
}

void RendererHandler::onStop()
{
    renderer_.releaseProducers();
}

void installRendererHandler(Renderer& renderer)
{
    Trackers::addHandler(std::make_shared<RendererHandler>(renderer));
}
