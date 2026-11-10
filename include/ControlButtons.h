// ControlButtons.h - The zoom, speed, step, restart, export and save buttons with their labels.

#pragma once

#include <string>

#include "HudCommon.h"

// The zoom, speed, step, restart, export and save buttons with their labels.
class ControlButtons
{
public:
    // Identifies each button; None means no button.
    enum Button
    {
        ZoomOut = 0,
        ZoomIn = 1,
        SpeedDown = 2,
        SpeedUp = 3,
        StepBack = 4,
        StepForward = 5,
        Restart = 6,
        ExportPng = 7,
        SaveRun = 8,
        None = -1
    };

    // Builds the button shapes and labels.
    void create(const HudContext& context);

    // Positions the buttons and refreshes the speed and step labels.
    void update(const HudContext& context, float speed, int shownEvents, int totalEvents);

    // Returns the button under the pixel (x, y), or None.
    int buttonAt(const HudContext& context, int x, int y) const;

    // Marks a button as held down.
    void setPressed(int button) { pressed_ = button; }

    // The button held down, or None.
    int pressed() const { return pressed_; }

private:
    // Replaces the text drawn in a label.
    void setLabel(Ogre::ManualObject* object, const std::string& text);

    Ogre::SceneNode* buttonNode_[hud::kControlButtons] = {};
    int pressed_ = None;

    Ogre::ManualObject* speedLabel_ = nullptr;
    Ogre::SceneNode* speedLabelNode_ = nullptr;
    Ogre::SceneNode* zoomLabelNode_ = nullptr;
    std::string speedLabelShown_;
    Ogre::ManualObject* stepLabel_ = nullptr;
    Ogre::SceneNode* stepLabelNode_ = nullptr;
    Ogre::SceneNode* restartLabelNode_ = nullptr;
    Ogre::SceneNode* exportLabelNode_ = nullptr;
    Ogre::SceneNode* saveLabelNode_ = nullptr;
    std::string stepLabelShown_;
};
