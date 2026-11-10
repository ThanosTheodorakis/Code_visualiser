// HudCommon.h - Sizes and window measurements shared by the parts of the on-screen interface.

#pragma once

#include "Ogre.h"
#include "configParser.h"

// Button sizes in pixels.
namespace hud {
constexpr float kButtonSize = 44.0f;
constexpr float kButtonMargin = 14.0f;
constexpr float kButtonDistance = 100.0f;
constexpr float kGap = 8.0f;
constexpr int kControlButtons = 9;
}

// The Ogre objects every interface part needs.
struct HudContext
{
    Ogre::SceneManager* scnMgr = nullptr;
    Ogre::SceneNode* camNode = nullptr;
    Ogre::Camera* cam = nullptr;
    Ogre::RenderWindow* window = nullptr;
    const ConfigData* config = nullptr;
};

// Window size in pixels and the world size of one pixel.
struct ScreenInfo
{
    float width = 0.0f;
    float height = 0.0f;
    float unitsPerPixel = 0.0f;
};

// Reads the window size; false if there is no window.
bool measureViewport(const HudContext& context, float& width, float& height);

// Reads the window size and the world size of one pixel.
bool measureScreen(const HudContext& context, ScreenInfo& screen);
