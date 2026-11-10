// InputController.h - Turns keyboard and mouse input into player and camera actions.

#pragma once

#include "OgreInput.h"

#include "CameraController.h"
#include "Hud.h"
#include "PlayerControls.h"

// Turns keyboard and mouse input into player, camera and button actions.
class InputController : public OgreBites::InputListener
{
public:
    InputController(PlayerControls& player, CameraController& camera, Hud& hud)
        : player_(player), camera_(camera), hud_(hud)
    {}

    // Handles the keyboard shortcuts.
    bool keyPressed(const OgreBites::KeyboardEvent& evt) override;
    // Tracks the Ctrl key.
    bool keyReleased(const OgreBites::KeyboardEvent& evt) override;
    // Drags the view while a mouse button is held.
    bool mouseMoved(const OgreBites::MouseMotionEvent& evt) override;
    // Handles clicks on the buttons and the start of a drag.
    bool mousePressed(const OgreBites::MouseButtonEvent& evt) override;
    // Ends a button press or a drag.
    bool mouseReleased(const OgreBites::MouseButtonEvent& evt) override;
    // Scrolls the view, or zooms while Ctrl is held.
    bool mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) override;

    // Repeats the zoom while a zoom button stays pressed.
    void update(float deltaSeconds);

private:
    PlayerControls& player_;
    CameraController& camera_;
    Hud& hud_;

    bool ctrlDown_ = false;
    bool pauseButtonDown_ = false;
    float zoomHeldSeconds_ = 0.0f;

    // Seconds a zoom button must be held before it repeats.
    static constexpr float kZoomHoldDelay = 0.35f;
    // Zoom speed while a zoom button is held.
    static constexpr float kZoomHoldRate = 1.6f;
};
