// PauseButton.h - The pause and resume button in the top right corner.

#pragma once

#include "HudCommon.h"

// The pause and resume button.
class PauseButton
{
public:
    // Builds the button.
    void create(const HudContext& context);

    // Shows the pause or resume symbol.
    void update(const HudContext& context, bool paused);

    // True if the pixel (x, y) is over the button.
    bool contains(const HudContext& context, int x, int y) const;

private:
    Ogre::SceneNode* node_ = nullptr;
    Ogre::ManualObject* frame_ = nullptr;
    Ogre::ManualObject* running_ = nullptr;
    Ogre::ManualObject* paused_ = nullptr;
};
