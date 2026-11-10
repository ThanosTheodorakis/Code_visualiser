// StatusIndicator.h - The Live, Halted, Replaying or Finished indicator in the top left corner.

#pragma once

#include "HudCommon.h"

// State shown by the indicator.
enum class PlaybackStatus
{
    Halted = 0,
    Live = 1,
    Replaying = 2,
    Finished = 3
};

// The indicator that shows the playback state.
class StatusIndicator
{
public:
    // Builds the indicator.
    void create(const HudContext& context);
    // Redraws the indicator when the state changed.
    void update(const HudContext& context, PlaybackStatus status);

private:
    Ogre::ManualObject* object_ = nullptr;
    Ogre::SceneNode* node_ = nullptr;
    int shown_ = -1;
};
