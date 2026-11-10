// CallStackPanel.cpp - Implementation of CallStackPanel.

#include "CallStackPanel.h"

#include <algorithm>
#include <cmath>

#include "ShapeGeometry.h"
#include "StrokeFont.h"

using namespace Ogre;

void CallStackPanel::create(const HudContext& context)
{
    if (!context.scnMgr || !context.camNode) {
        return;
    }

    object_ = context.scnMgr->createManualObject("FvCallStack");
    object_->setDynamic(true);
    node_ = context.camNode->createChildSceneNode();
    node_->attachObject(object_);
}

void CallStackPanel::update(const HudContext& context, const std::vector<std::string>& callStack,
                            bool legendVisible, float legendHeight)
{
    ScreenInfo screen;
    if (!object_ || !node_ || !measureScreen(context, screen)) {
        return;
    }

    const float width = screen.width;
    const float height = screen.height;

    const float top = hud::kButtonMargin + 28.0f + 10.0f;
    const float bottomReserve = hud::kButtonMargin + (legendVisible ? legendHeight + 10.0f : 0.0f);

    std::string key = visible_ ? "v" : "h";
    key += std::to_string(static_cast<int>(width)) + "x" + std::to_string(static_cast<int>(height)) +
           (legendVisible ? "L" : "l");
    for (const std::string& name : callStack) {
        key += "|" + name;
    }
    if (key == shown_) {
        return;
    }
    shown_ = key;

    node_->setVisible(visible_);
    if (!visible_) {
        return;
    }

    const ConfigData& config = *context.config;
    const float textHeight = 11.0f;
    const float pitch = 18.0f;
    const float pad = 9.0f;
    const float panelWidth = 250.0f;
    const float maxTextWidth = panelWidth - pad * 2.0f - 12.0f;
    const float titleHeight = pitch + 4.0f;

    auto textWidth = [&](const std::string& text) {
        float widest = 0.0f;
        for (const strokefont::Segment& segment : strokefont::layout(text, textHeight)) {
            widest = std::max(widest, std::max(segment.x1, segment.x2));
        }
        return widest;
    };

    auto fit = [&](std::string text) {
        if (textWidth(text) <= maxTextWidth) {
            return text;
        }

        std::string tail;
        const std::size_t paren = text.rfind('(');
        if (paren != std::string::npos && paren > 0) {
            tail = text.substr(paren);
            if (textWidth("..." + tail) > maxTextWidth * 0.6f) {
                tail.clear();
            } else {
                text.erase(paren);
            }
        }

        while (!text.empty() && textWidth(text + "..." + tail) > maxTextWidth) {
            text.pop_back();
        }
        return text + "..." + tail;
    };

    const int depth = static_cast<int>(callStack.size());
    const float available = height - top - bottomReserve - pad * 2.0f - titleHeight;
    int rows = std::max(1, static_cast<int>(available / pitch));

    std::vector<std::string> lines;
    if (depth == 0) {
        lines.push_back("(empty)");
    } else if (depth <= rows) {
        for (int i = depth - 1; i >= 0; --i) {
            lines.push_back(callStack[i]);
        }
    } else {
        for (int i = 0; i < rows - 1; ++i) {
            lines.push_back(callStack[depth - 1 - i]);
        }
        lines.push_back("... " + std::to_string(depth - (rows - 1)) + " more");
    }

    const float panelHeight = pad * 2.0f + titleHeight + pitch * lines.size();

    object_->clear();
    object_->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);
    int index = 0;

    const ColourValue frameColour(0.40f, 0.42f, 0.48f);
    shapes::rect(object_, index, 0.0f, -panelHeight, panelWidth, 0.0f, 0.0f,
                 ColourValue(0.06f, 0.07f, 0.10f));
    shapes::rect(object_, index, 0, -panelHeight, panelWidth, -panelHeight + 1, 1.0f, frameColour);
    shapes::rect(object_, index, 0, -1, panelWidth, 0, 1.0f, frameColour);
    shapes::rect(object_, index, 0, -panelHeight, 1, 0, 1.0f, frameColour);
    shapes::rect(object_, index, panelWidth - 1, -panelHeight, panelWidth, 0, 1.0f, frameColour);

    auto drawText = [&](const std::string& text, float x, float centreY, const ColourValue& colour) {
        const float originY = centreY - textHeight * 0.5f;
        for (const strokefont::Segment& segment : strokefont::layout(text, textHeight)) {
            shapes::thickLine(object_, index, x + segment.x1, originY + segment.y1, x + segment.x2,
                              originY + segment.y2, 1.4f, 2.0f, colour, true);
        }
    };

    const ColourValue titleColour(0.62f, 0.66f, 0.74f);
    const ColourValue textColour(0.88f, 0.89f, 0.92f);
    drawText("call stack  (" + std::to_string(depth) + ")", pad, -pad - pitch * 0.5f, titleColour);
    shapes::rect(object_, index, pad, -pad - titleHeight + 1.0f, panelWidth - pad, -pad - titleHeight + 2.0f,
                 2.0f, frameColour);

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const float centreY = -pad - titleHeight - pitch * (i + 0.5f);
        const bool innermost = depth > 0 && i == 0;
        if (innermost) {
            shapes::triangleFill(object_, index, pad, centreY + 4.5f, pad, centreY - 4.5f, pad + 7.0f,
                                 centreY, 2.0f, config.functionLineColor);
        }
        drawText(fit(lines[i]), pad + 12.0f, centreY, innermost ? config.functionLineColor : textColour);
    }

    object_->end();

    const float unitsPerPixel = screen.unitsPerPixel;

    node_->setPosition((hud::kButtonMargin - width * 0.5f) * unitsPerPixel,
                       (height * 0.5f - top) * unitsPerPixel, -hud::kButtonDistance);
    node_->setScale(unitsPerPixel, unitsPerPixel, unitsPerPixel);
}
