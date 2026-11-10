// InputController.cpp - Implementation of InputController.

#include "InputController.h"

#include <cmath>

using namespace OgreBites;

namespace {
const int kKeyLeft = 1073741904;
const int kKeyRight = 1073741903;

bool isCtrlKey(int sym)
{
    return sym == 1073742048 || sym == 1073742052;
}
}

bool InputController::keyReleased(const KeyboardEvent& event)
{
    if (isCtrlKey(event.keysym.sym)) {
        ctrlDown_ = false;
    }
    return true;
}

bool InputController::keyPressed(const KeyboardEvent& event)
{
    const auto key = event.keysym.sym;

    if (isCtrlKey(key)) {
        ctrlDown_ = true;
        return true;
    }

    if (key == SDLK_ESCAPE) {
        player_.quitApplication();
    } else if (key == SDLK_SPACE) {
        player_.togglePause();
    } else if (key == 'f') {
        camera_.followLatest();
    } else if (key == 'r') {
        player_.restart();
    } else if (key == 'e') {
        player_.exportImage();
    } else if (key == 's') {
        player_.saveRunNow();
    } else if (key == 'c') {
        player_.toggleCallStack();
    } else if (key == kKeyLeft) {
        player_.stepBack();
    } else if (key == kKeyRight) {
        player_.stepForward();
    } else if (key == 'l') {
        player_.toggleLegend();
    } else if (key == '=' || key == '+' || key == SDLK_KP_PLUS) {
        player_.speedUp();
    } else if (key == '-' || key == SDLK_KP_MINUS) {
        player_.slowDown();
    }

    return true;
}

bool InputController::mouseWheelRolled(const MouseWheelEvent& event)
{
    if (!camera_.attached()) {
        return false;
    }

    if (ctrlDown_) {
        camera_.zoomWheel(event.y);
        return true;
    }

    camera_.scrollWheel(event.y);
    return true;
}

bool InputController::mousePressed(const MouseButtonEvent& event)
{
    if (event.button == BUTTON_LEFT) {
        const int button = hud_.controlButtonAt(event.x, event.y);
        if (button >= 0) {
            hud_.setControlPressed(button);
            zoomHeldSeconds_ = 0.0f;

            if (button < 2) {
                camera_.zoomBy(button == 1 ? CameraController::kZoomStep
                                           : 1.0f / CameraController::kZoomStep);
            } else if (button < 4) {
                if (button == 3) {
                    player_.speedUp();
                } else {
                    player_.slowDown();
                }
            } else if (button == ControlButtons::StepBack) {
                player_.stepBack();
            } else if (button == ControlButtons::StepForward) {
                player_.stepForward();
            } else if (button == ControlButtons::Restart) {
                player_.restart();
            } else if (button == ControlButtons::ExportPng) {
                player_.exportImage();
            } else {
                player_.saveRunNow();
            }

            player_.refreshControls();
            return true;
        }
    }

    if (event.button == BUTTON_LEFT && hud_.overPauseButton(event.x, event.y)) {
        pauseButtonDown_ = true;
        return true;
    }

    if ((event.button == BUTTON_LEFT || event.button == BUTTON_RIGHT) && camera_.startDrag(event.button)) {
        return true;
    }

    return false;
}

bool InputController::mouseReleased(const MouseButtonEvent& event)
{
    if (event.button == BUTTON_LEFT && hud_.pressedControl() >= 0) {
        hud_.setControlPressed(ControlButtons::None);
        player_.refreshControls();
        return true;
    }

    if (event.button == BUTTON_LEFT && pauseButtonDown_) {
        pauseButtonDown_ = false;
        if (hud_.overPauseButton(event.x, event.y)) {
            player_.togglePause();
        }
        return true;
    }

    if (camera_.endDrag(event.button)) {
        return true;
    }

    return false;
}

bool InputController::mouseMoved(const MouseMotionEvent& event)
{
    if (!camera_.dragging() || !camera_.attached()) {
        return false;
    }

    camera_.dragBy(event.xrel, event.yrel);
    return true;
}

void InputController::update(float deltaSeconds)
{
    const int pressed = hud_.pressedControl();
    if (pressed >= 0 && pressed < 2) {
        zoomHeldSeconds_ += deltaSeconds;
        if (zoomHeldSeconds_ > kZoomHoldDelay) {
            const float change = std::exp(kZoomHoldRate * deltaSeconds);
            camera_.zoomBy(pressed == 1 ? 1.0f / change : change);
        }
    }
}
