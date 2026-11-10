// PlayerControls.h - The actions the user can ask the player to do; implemented by the renderer.

#pragma once

// The actions the user can ask the player to do; the renderer implements them.
class PlayerControls
{
public:
    virtual ~PlayerControls() {}

    // Pauses or resumes playback.
    virtual void togglePause() = 0;
    // Starts again from the first event.
    virtual void restart() = 0;
    // Pauses and goes one event back.
    virtual void stepBack() = 0;
    // Pauses and shows the next event.
    virtual void stepForward() = 0;
    // Doubles the playback speed.
    virtual void speedUp() = 0;
    // Halves the playback speed.
    virtual void slowDown() = 0;
    // Saves a screenshot of the current view.
    virtual void exportImage() = 0;
    // Saves the recorded run to the runs folder.
    virtual void saveRunNow() = 0;
    // Shows or hides the call stack panel.
    virtual void toggleCallStack() = 0;
    // Shows or hides the legend.
    virtual void toggleLegend() = 0;

    // Redraws the button labels.
    virtual void refreshControls() = 0;

    // Closes the window.
    virtual void quitApplication() = 0;
};
