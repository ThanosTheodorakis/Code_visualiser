// Hud.h - The on-screen interface that sits in front of the chart.

#pragma once

#include <string>
#include <vector>

#include "CallStackPanel.h"
#include "ControlButtons.h"
#include "HudCommon.h"
#include "Legend.h"
#include "PauseButton.h"
#include "StatusIndicator.h"

// The on-screen interface that sits in front of the chart.
class Hud
{
public:
    // Builds all interface parts.
    void create(const HudContext& context);

    // Shows the pause or resume symbol.
    void updatePauseButton(bool paused) { pause_.update(context_, paused); }

    // Refreshes the speed and step labels.
    void updateControls(float speed, int shownEvents, int totalEvents)
    {
        controls_.update(context_, speed, shownEvents, totalEvents);
    }

    // Keeps the legend in place.
    void updateLegend() { legend_.update(context_); }

    // Shows the playback state.
    void updateStatus(PlaybackStatus status) { status_.update(context_, status); }

    // Shows the open function calls next to the legend.
    void updateCallStack(const std::vector<std::string>& frames)
    {
        callStack_.update(context_, frames, legend_.visible(), legend_.height());
    }

    // Shows or hides the legend.
    void toggleLegend() { legend_.toggle(); }

    // Shows or hides the call stack panel.
    void toggleCallStack() { callStack_.toggle(); }

    // Control button under the pixel, or none.
    int controlButtonAt(int x, int y) const { return controls_.buttonAt(context_, x, y); }

    // True if the pixel is over the pause button.
    bool overPauseButton(int x, int y) const { return pause_.contains(context_, x, y); }

    // Marks a control button as held down.
    void setControlPressed(int button) { controls_.setPressed(button); }

    // The control button held down, or none.
    int pressedControl() const { return controls_.pressed(); }

private:
    HudContext context_;
    PauseButton pause_;
    ControlButtons controls_;
    StatusIndicator status_;
    Legend legend_;
    CallStackPanel callStack_;
};
