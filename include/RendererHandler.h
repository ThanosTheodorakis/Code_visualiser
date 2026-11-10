// RendererHandler.h - Handler that translates marker events into renderer events.

#pragma once

#include <memory>
#include <string>

#include "Markers.h"

class Renderer;

// Receives the markers' events and enqueues them in the renderer. Its methods run on the program's thread.
// Before adding an event they call waitForCapacity(), which blocks until the renderer has caught up, so the
// program advances only as fast as it is drawn. The state in RendererHandler.cpp is per thread (or atomic)
// because only the program thread uses it.
class RendererHandler : public TrackerHandler
{
public:
    // The handler enqueues its events in the given renderer.
    explicit RendererHandler(Renderer& renderer) : renderer_(renderer) {}

    void onFunctionEnter(const std::string& name) override;
    void onFunctionExit(const std::string& name) override;
    void onReturn(const std::string& value) override;
    using TrackerHandler::onBranchEnter;
    void onBranchEnter(bool condition, const std::string& text) override;
    void onBranchExit(bool condition) override;
    void onDecision(const std::string& text, bool value) override;
    void onSwitchEnter(const std::string& expression, const std::string& value) override;
    void onSwitchExit() override;
    void onLoopEnter(const std::string& loopType) override;
    void onLoopExit(const std::string& loopType) override;
    void onBlockEnter(const std::string& name) override;
    void onBlockExit(const std::string& name) override;

    void onLoopIteration(const std::string& loopType, int index) override;
    void onStatement(const std::string& text) override;

    void onProgramStart() override;
    void onProgramEnd() override;
    void onInput(const std::string& text) override;
    void onInputDone(const std::string& text) override;
    void onOutput(const std::string& text) override;

    void onStop() override;

private:
    Renderer& renderer_;
};

// Registers a RendererHandler that feeds the given renderer.
void installRendererHandler(Renderer& renderer);
