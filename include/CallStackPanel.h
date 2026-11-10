// CallStackPanel.h - The on-screen panel that lists the open function calls.

#pragma once

#include <string>
#include <vector>

#include "HudCommon.h"

// The panel that lists the open function calls.
class CallStackPanel
{
public:
    // Builds the panel objects.
    void create(const HudContext& context);

    // Redraws the panel when the list of calls changed.
    void update(const HudContext& context, const std::vector<std::string>& frames, bool legendVisible,
                float legendHeight);

    // True while the panel is shown.
    bool visible() const { return visible_; }

    // Shows or hides the panel.
    void toggle()
    {
        visible_ = !visible_;
        shown_.clear();
    }

private:
    Ogre::SceneNode* node_ = nullptr;
    Ogre::ManualObject* object_ = nullptr;
    bool visible_ = true;
    std::string shown_;
};
