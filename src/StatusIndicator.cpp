// StatusIndicator.cpp - Implementation of StatusIndicator.

#include "StatusIndicator.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "ShapeGeometry.h"
#include "StrokeFont.h"

using namespace Ogre;

void StatusIndicator::create(const HudContext& context)
{
    if (!context.scnMgr || !context.camNode) {
        return;
    }

    object_ = context.scnMgr->createManualObject("FvStatus");
    object_->setDynamic(true);
    node_ = context.camNode->createChildSceneNode();
    node_->attachObject(object_);
}

void StatusIndicator::update(const HudContext& context, PlaybackStatus status)
{
    ScreenInfo screen;
    if (!object_ || !measureScreen(context, screen)) {
        return;
    }

    const float width = screen.width;
    const float height = screen.height;
    const int state = static_cast<int>(status);

    const bool live = state == 1;

    if (shown_ != state) {
        shown_ = state;

        static const char* const words[4] = {"Halted", "Live", "Replaying", "Finished"};
        const std::string text = std::string("Program execution: ") + words[state];

        const float textHeight = 12.0f;
        const std::vector<strokefont::Segment> segments = strokefont::layout(text, textHeight);

        float maxX = 0.0f;
        for (const strokefont::Segment& segment : segments) {
            maxX = std::max(maxX, std::max(segment.x1, segment.x2));
        }

        const float plateHeight = 28.0f;
        const float textX = 30.0f;
        const ColourValue dots[4] = {ColourValue(0.50f, 0.50f, 0.54f), ColourValue(0.95f, 0.12f, 0.12f),
                                     ColourValue(1.00f, 0.70f, 0.15f), ColourValue(0.30f, 0.85f, 0.45f)};
        const ColourValue dot = dots[state];

        object_->clear();
        object_->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

        int index = 0;

        shapes::rect(object_, index, 0.0f, 0.0f, textX + maxX + 12.0f, -plateHeight, 0.0f,
                     ColourValue(0.06f, 0.07f, 0.10f));

        if (live) {
            shapes::ellipseBand(object_, index, 15.0f, -14.0f, 9.5f, 9.5f, 0.0f, 2.0f * shapes::kPi, 1.8f,
                                1.0f, ColourValue(0.55f, 0.08f, 0.08f), 32);
        }

        shapes::ellipseFan(object_, index, 15.0f, -14.0f, 6.0f, 6.0f, 0.0f, 2.0f * shapes::kPi, 2.0f, dot,
                           32);

        for (const strokefont::Segment& segment : segments) {
            shapes::thickLine(
                object_, index, textX + segment.x1, -14.0f - textHeight * 0.5f + segment.y1,
                textX + segment.x2, -14.0f - textHeight * 0.5f + segment.y2, 1.5f, 2.0f,
                state == 0 ? ColourValue(0.60f, 0.61f, 0.65f) : ColourValue(0.92f, 0.93f, 0.95f), true);
        }

        object_->end();
    }

    const float unitsPerPixel = screen.unitsPerPixel;

    node_->setPosition((hud::kButtonMargin - width * 0.5f) * unitsPerPixel,
                       (height * 0.5f - hud::kButtonMargin) * unitsPerPixel, -hud::kButtonDistance);
    node_->setScale(unitsPerPixel, unitsPerPixel, unitsPerPixel);
}
