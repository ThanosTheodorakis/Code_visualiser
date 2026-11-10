// HudCommon.cpp - Window measurements for the on-screen interface.

#include "HudCommon.h"

#include <cmath>

bool measureViewport(const HudContext& context, float& width, float& height)
{
    if (!context.window) {
        return false;
    }

    Ogre::Viewport* viewport = context.window->getViewport(0);
    if (!viewport) {
        return false;
    }

    width = static_cast<float>(viewport->getActualWidth());
    height = static_cast<float>(viewport->getActualHeight());
    return true;
}

bool measureScreen(const HudContext& context, ScreenInfo& screen)
{
    if (!context.cam || !measureViewport(context, screen.width, screen.height)) {
        return false;
    }
    if (screen.width <= 0.0f || screen.height <= 0.0f) {
        return false;
    }

    screen.unitsPerPixel =
        2.0f * std::tan(context.cam->getFOVy().valueRadians() * 0.5f) * hud::kButtonDistance / screen.height;
    return true;
}
