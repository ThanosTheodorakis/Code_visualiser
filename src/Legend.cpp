// Legend.cpp - Implementation of Legend.

#include "Legend.h"

#include <algorithm>
#include <cmath>

#include "ShapeGeometry.h"
#include "StrokeFont.h"

using namespace Ogre;

void Legend::create(const HudContext& context)
{
    SceneManager* scnMgr = context.scnMgr;
    SceneNode* camNode = context.camNode;
    if (!scnMgr || !camNode || !context.config) {
        return;
    }

    const ConfigData& config = *context.config;

    struct Entry
    {
        const char* text;
        ColourValue colour;
        bool dashed;
        bool parallelogram;
        bool terminator;
    };

    const Entry entries[] = {
        {"function", config.functionLineColor, false, false, false},
        {"branch", config.branchLineColor, false, false, false},
        {"loop", config.loopLineColor, false, false, false},
        {"statement", config.statementLineColor, false, false, false},
        {"flow", config.flowLineColor, false, false, false},
        {"jump back", config.connectorLineColor, true, false, false},
        {"input", config.inputLineColor, false, true, false},
        {"output", config.outputLineColor, false, true, false},
        {"start / end", config.terminalLineColor, false, false, true},
    };
    const int count = static_cast<int>(sizeof(entries) / sizeof(entries[0]));

    const float textHeight = 11.0f;
    const float pitch = 18.0f;
    const float pad = 9.0f;
    const float swatchWidth = 26.0f;
    const float gap = 8.0f;

    float widest = 0.0f;
    for (int i = 0; i < count; ++i) {
        for (const strokefont::Segment& segment : strokefont::layout(entries[i].text, textHeight)) {
            widest = std::max(widest, std::max(segment.x2, segment.x1));
        }
    }

    width_ = pad * 2.0f + swatchWidth + gap + widest;
    height_ = pad * 2.0f + pitch * count;

    ManualObject* legend = scnMgr->createManualObject("FvLegend");
    legend->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    const ColourValue frameColour(0.40f, 0.42f, 0.48f);
    shapes::rect(legend, index, 0.0f, 0.0f, width_, height_, 0.0f, ColourValue(0.06f, 0.07f, 0.10f));

    shapes::rect(legend, index, 0, 0, width_, 1, 1.0f, frameColour);
    shapes::rect(legend, index, 0, height_ - 1, width_, height_, 1.0f, frameColour);
    shapes::rect(legend, index, 0, 0, 1, height_, 1.0f, frameColour);
    shapes::rect(legend, index, width_ - 1, 0, width_, height_, 1.0f, frameColour);

    const ColourValue textColour(0.88f, 0.89f, 0.92f);

    for (int i = 0; i < count; ++i) {
        const float centreY = height_ - pad - pitch * (i + 0.5f);

        if (entries[i].terminator) {
            shapes::ellipseFan(legend, index, pad + swatchWidth * 0.5f, centreY, swatchWidth * 0.5f, 5.5f,
                               0.0f, 2.0f * shapes::kPi, 2.0f, entries[i].colour, 24);
        } else if (entries[i].parallelogram) {
            const float hh = 5.0f;
            const float sk = 6.0f;
            shapes::quad(legend, index, pad + sk, centreY + hh, pad + swatchWidth, centreY + hh,
                         pad + swatchWidth - sk, centreY - hh, pad, centreY - hh, 2.0f, entries[i].colour);
        } else if (entries[i].dashed) {
            shapes::dashedLine(legend, index, pad, centreY, pad + swatchWidth, centreY, 3.0f, 6.0f, 4.0f,
                               2.0f, entries[i].colour);
        } else {
            shapes::thickLine(legend, index, pad, centreY, pad + swatchWidth, centreY, 3.0f, 2.0f,
                              entries[i].colour, false);
        }

        const float originX = pad + swatchWidth + gap;
        const float originY = centreY - textHeight * 0.5f;

        for (const strokefont::Segment& segment : strokefont::layout(entries[i].text, textHeight)) {
            shapes::thickLine(legend, index, originX + segment.x1, originY + segment.y1, originX + segment.x2,
                              originY + segment.y2, 1.4f, 2.0f, textColour, true);
        }
    }

    legend->end();

    node_ = camNode->createChildSceneNode();
    node_->attachObject(legend);
}

void Legend::update(const HudContext& context)
{
    ScreenInfo screen;
    if (!node_ || !measureScreen(context, screen)) {
        return;
    }

    const float width = screen.width;
    const float height = screen.height;
    const float unitsPerPixel = screen.unitsPerPixel;

    const float px = hud::kButtonMargin;
    const float py = height - hud::kButtonMargin;

    node_->setPosition((px - width * 0.5f) * unitsPerPixel, (height * 0.5f - py) * unitsPerPixel,
                       -hud::kButtonDistance);
    node_->setScale(unitsPerPixel, unitsPerPixel, unitsPerPixel);
    node_->setVisible(visible_);
}
