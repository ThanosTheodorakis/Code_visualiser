// PauseButton.cpp - Implementation of PauseButton.

#include "PauseButton.h"

#include <cmath>

#include "ShapeGeometry.h"

using namespace Ogre;

namespace {
void addQuad(ManualObject* object, float x0, float y0, float x1, float y1, float z, const ColourValue& colour,
             int& index)
{
    const float xs[4] = {x0, x1, x1, x0};
    const float ys[4] = {y0, y0, y1, y1};
    for (int i = 0; i < 4; ++i) {
        object->position(xs[i], ys[i], z);
        object->colour(colour);
    }
    object->triangle(index, index + 1, index + 2);
    object->triangle(index, index + 2, index + 3);
    object->triangle(index, index + 2, index + 1);
    object->triangle(index, index + 3, index + 2);
    index += 4;
}

void addTriangle(ManualObject* object, float x0, float y0, float x1, float y1, float x2, float y2, float z,
                 const ColourValue& colour, int& index)
{
    object->position(x0, y0, z);
    object->colour(colour);
    object->position(x1, y1, z);
    object->colour(colour);
    object->position(x2, y2, z);
    object->colour(colour);
    object->triangle(index, index + 1, index + 2);
    object->triangle(index, index + 2, index + 1);
    index += 3;
}
}

void PauseButton::create(const HudContext& context)
{
    SceneManager* scnMgr = context.scnMgr;
    if (!scnMgr || !context.camNode) {
        return;
    }

    const float h = hud::kButtonSize * 0.5f;
    const ColourValue frameColour(0.85f, 0.85f, 0.90f);
    const ColourValue iconColour(1.0f, 1.0f, 1.0f);
    const float zBackground = 0.0f;
    const float zFrame = 2.0f;
    const float zIcon = 4.0f;

    frame_ = scnMgr->createManualObject("FvPauseButtonFrame");
    frame_->begin("BaseWhiteNoLighting", RenderOperation::OT_LINE_STRIP);
    const float cx[5] = {-h, h, h, -h, -h};
    const float cy[5] = {-h, -h, h, h, -h};
    for (int i = 0; i < 5; ++i) {
        frame_->position(cx[i], cy[i], zFrame);
        frame_->colour(frameColour);
    }
    frame_->end();

    running_ = scnMgr->createManualObject("FvPauseButtonRunning");
    running_->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);
    {
        int index = 0;
        addQuad(running_, -h, -h, h, h, zBackground, ColourValue(0.10f, 0.11f, 0.15f), index);
        addQuad(running_, -10.0f, -10.0f, -3.0f, 10.0f, zIcon, iconColour, index);
        addQuad(running_, 3.0f, -10.0f, 10.0f, 10.0f, zIcon, iconColour, index);
    }
    running_->end();

    paused_ = scnMgr->createManualObject("FvPauseButtonPaused");
    paused_->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);
    {
        int index = 0;
        addQuad(paused_, -h, -h, h, h, zBackground, ColourValue(0.70f, 0.32f, 0.05f), index);
        addTriangle(paused_, -7.0f, -11.0f, 11.0f, 0.0f, -7.0f, 11.0f, zIcon, iconColour, index);
    }
    paused_->end();
    paused_->setVisible(false);

    node_ = context.camNode->createChildSceneNode();
    node_->attachObject(frame_);
    node_->attachObject(running_);
    node_->attachObject(paused_);
}

void PauseButton::update(const HudContext& context, bool paused)
{
    ScreenInfo screen;
    if (!node_ || !measureScreen(context, screen)) {
        return;
    }

    const float width = screen.width;
    const float height = screen.height;
    const float unitsPerPixel = screen.unitsPerPixel;

    const float px = width - hud::kButtonMargin - hud::kButtonSize * 0.5f;
    const float py = hud::kButtonMargin + hud::kButtonSize * 0.5f;

    node_->setPosition((px - width * 0.5f) * unitsPerPixel, (height * 0.5f - py) * unitsPerPixel,
                       -hud::kButtonDistance);
    node_->setScale(unitsPerPixel, unitsPerPixel, unitsPerPixel);

    running_->setVisible(!paused);
    paused_->setVisible(paused);
}

bool PauseButton::contains(const HudContext& context, int x, int y) const
{
    float width = 0.0f;
    float height = 0.0f;
    if (!measureViewport(context, width, height)) {
        return false;
    }

    const float left = width - hud::kButtonMargin - hud::kButtonSize;
    const float top = hud::kButtonMargin;

    return x >= left && x <= left + hud::kButtonSize && y >= top && y <= top + hud::kButtonSize;
}
