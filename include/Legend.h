// Legend.h - The colour legend in the bottom left corner.

#pragma once

#include "HudCommon.h"

// The colour legend that explains the line colours.
class Legend
{
public:
    // Builds the legend.
    void create(const HudContext& context);
    // Keeps the legend at the window corner.
    void update(const HudContext& context);

    // True while the legend is shown.
    bool visible() const { return visible_; }

    // Shows or hides the legend.
    void toggle() { visible_ = !visible_; }

    // Height of the legend, so other panels can be placed next to it.
    float height() const { return height_; }

private:
    Ogre::SceneNode* node_ = nullptr;
    bool visible_ = true;
    float width_ = 0.0f;
    float height_ = 0.0f;
};
