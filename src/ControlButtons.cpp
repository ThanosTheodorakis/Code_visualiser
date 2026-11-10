// ControlButtons.cpp - Implementation of ControlButtons.

#include "ControlButtons.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "ShapeGeometry.h"
#include "StrokeFont.h"

using namespace Ogre;

void ControlButtons::create(const HudContext& context)
{
    SceneManager* scnMgr = context.scnMgr;
    SceneNode* camNode = context.camNode;
    if (!scnMgr || !camNode) {
        return;
    }

    const float h = hud::kButtonSize * 0.5f;
    const ColourValue background(0.10f, 0.11f, 0.15f);
    const ColourValue frame(0.85f, 0.85f, 0.90f);
    const ColourValue icon(1.0f, 1.0f, 1.0f);
    const char* const names[hud::kControlButtons] = {"FvZoomOut", "FvZoomIn",   "FvSpeedDown",
                                                     "FvSpeedUp", "FvStepBack", "FvStepForward",
                                                     "FvRestart", "FvExport",   "FvSaveRun"};

    for (int i = 0; i < hud::kControlButtons; ++i) {
        ManualObject* object = scnMgr->createManualObject(names[i]);

        object->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

        int index = 0;

        shapes::rect(object, index, -h, -h, h, h, 0.0f, background);

        shapes::rect(object, index, -h, -h, h, -h + 1.0f, 2.0f, frame);
        shapes::rect(object, index, -h, h - 1.0f, h, h, 2.0f, frame);
        shapes::rect(object, index, -h, -h, -h + 1.0f, h, 2.0f, frame);
        shapes::rect(object, index, h - 1.0f, -h, h, h, 2.0f, frame);

        if (i < 2) {
            shapes::rect(object, index, -10.0f, -1.5f, 10.0f, 1.5f, 4.0f, icon);
            if (i == 1) {
                shapes::rect(object, index, -1.5f, -10.0f, 1.5f, 10.0f, 4.0f, icon);
            }
        } else if (i < 4) {
            const float d = (i == 3) ? 1.0f : -1.0f;
            shapes::triangleFill(object, index, -11.0f * d, -8.0f, -1.0f * d, 0.0f, -11.0f * d, 8.0f, 4.0f,
                                 icon);
            shapes::triangleFill(object, index, 0.0f * d, -8.0f, 10.0f * d, 0.0f, 0.0f * d, 8.0f, 4.0f, icon);
        } else if (i < 6) {
            const float d = (i == 5) ? 1.0f : -1.0f;
            shapes::triangleFill(object, index, 7.0f * d, 0.0f, -9.0f * d, -9.0f, -9.0f * d, 9.0f, 4.0f,
                                 icon);
            shapes::rect(object, index, std::min(8.0f * d, 11.0f * d), -9.0f, std::max(8.0f * d, 11.0f * d),
                         9.0f, 4.0f, icon);
        } else if (i == 8) {
            shapes::rect(object, index, -10.0f, -10.0f, 10.0f, -8.5f, 4.0f, icon);
            shapes::rect(object, index, -10.0f, 8.5f, 7.0f, 10.0f, 4.0f, icon);
            shapes::rect(object, index, -10.0f, -10.0f, -8.5f, 10.0f, 4.0f, icon);
            shapes::rect(object, index, 8.5f, -10.0f, 10.0f, 7.0f, 4.0f, icon);
            shapes::triangleFill(object, index, 7.0f, 10.0f, 10.0f, 7.0f, 7.0f, 7.0f, 4.0f, icon);
            shapes::rect(object, index, -5.0f, 3.0f, 4.0f, 10.0f, 4.0f, icon);
            shapes::rect(object, index, -6.0f, -8.5f, 6.0f, -1.0f, 4.0f, icon);
        } else if (i == 7) {
            shapes::rect(object, index, -11.0f, -8.0f, 11.0f, -6.5f, 4.0f, icon);
            shapes::rect(object, index, -11.0f, 6.5f, 11.0f, 8.0f, 4.0f, icon);
            shapes::rect(object, index, -11.0f, -8.0f, -9.5f, 8.0f, 4.0f, icon);
            shapes::rect(object, index, 9.5f, -8.0f, 11.0f, 8.0f, 4.0f, icon);
            shapes::triangleFill(object, index, -9.0f, -6.0f, -3.0f, 2.0f, 3.0f, -6.0f, 4.0f, icon);
            shapes::triangleFill(object, index, -1.0f, -6.0f, 4.0f, 0.0f, 9.0f, -6.0f, 4.0f, icon);
            shapes::diamond(object, index, 5.0f, 3.5f, 2.2f, 4.0f, icon);
        } else {
            const float start = 0.7f;
            const float end = start + 5.0f;
            shapes::ellipseBand(object, index, 0.0f, 0.0f, 9.0f, 9.0f, start, end, 3.0f, 4.0f, icon);
            shapes::arrowHead(object, index, 9.0f * std::cos(end) - 7.0f * std::sin(end),
                              9.0f * std::sin(end) + 7.0f * std::cos(end), -std::sin(end), std::cos(end),
                              9.0f, 4.0f, icon);
        }

        object->end();

        buttonNode_[i] = camNode->createChildSceneNode();
        buttonNode_[i]->attachObject(object);
    }

    ManualObject* zoomLabel = scnMgr->createManualObject("FvZoomLabel");
    setLabel(zoomLabel, "zoom");
    zoomLabelNode_ = camNode->createChildSceneNode();
    zoomLabelNode_->attachObject(zoomLabel);

    speedLabel_ = scnMgr->createManualObject("FvSpeedLabel");
    speedLabel_->setDynamic(true);
    speedLabelNode_ = camNode->createChildSceneNode();
    speedLabelNode_->attachObject(speedLabel_);

    stepLabel_ = scnMgr->createManualObject("FvStepLabel");
    stepLabel_->setDynamic(true);
    stepLabelNode_ = camNode->createChildSceneNode();
    stepLabelNode_->attachObject(stepLabel_);

    ManualObject* restartLabel = scnMgr->createManualObject("FvRestartLabel");
    setLabel(restartLabel, "restart");
    restartLabelNode_ = camNode->createChildSceneNode();
    restartLabelNode_->attachObject(restartLabel);

    ManualObject* exportLabel = scnMgr->createManualObject("FvExportLabel");
    setLabel(exportLabel, "export png");
    exportLabelNode_ = camNode->createChildSceneNode();
    exportLabelNode_->attachObject(exportLabel);

    ManualObject* saveLabel = scnMgr->createManualObject("FvSaveLabel");
    setLabel(saveLabel, "save run");
    saveLabelNode_ = camNode->createChildSceneNode();
    saveLabelNode_->attachObject(saveLabel);
}

void ControlButtons::setLabel(ManualObject* object, const std::string& text)
{
    const float height = 12.0f;
    const std::vector<strokefont::Segment> segments = strokefont::layout(text, height);

    float maxX = 0.0f;
    for (const strokefont::Segment& segment : segments) {
        maxX = std::max(maxX, std::max(segment.x1, segment.x2));
    }

    object->clear();
    object->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    shapes::rect(object, index, -maxX - 7.0f, -10.0f, 4.0f, 10.0f, 0.0f, ColourValue(0.06f, 0.07f, 0.10f));

    for (const strokefont::Segment& segment : segments) {
        shapes::thickLine(object, index, segment.x1 - maxX, segment.y1 - height * 0.5f, segment.x2 - maxX,
                          segment.y2 - height * 0.5f, 1.5f, 2.0f, ColourValue(0.88f, 0.89f, 0.92f), true);
    }

    shapes::ensureNotEmpty(object, index, ColourValue(0.0f, 0.0f, 0.0f));
    object->end();
}

static void hudButtonCentre(int id, float width, float height, float size, float margin, float gap,
                            float& centreX, float& centreY)
{
    const int row = (id == 8) ? 5 : (id == 7) ? 4 : (id == 6) ? 3 : id / 2;
    const int column = (id >= 6) ? 1 : id % 2;
    centreX = width - margin - size * 0.5f - (1 - column) * (size + gap);
    centreY = height - margin - size * 0.5f - row * (size + gap);
}

void ControlButtons::update(const HudContext& context, float speed, int shownEvents, int totalEvents)
{
    ScreenInfo screen;
    if (!buttonNode_[0] || !measureScreen(context, screen)) {
        return;
    }

    const float width = screen.width;
    const float height = screen.height;
    const float unitsPerPixel = screen.unitsPerPixel;

    for (int i = 0; i < hud::kControlButtons; ++i) {
        float centreX;
        float centreY;
        hudButtonCentre(i, width, height, hud::kButtonSize, hud::kButtonMargin, hud::kGap, centreX, centreY);

        const float scale = unitsPerPixel * (pressed_ == i ? 0.9f : 1.0f);

        buttonNode_[i]->setPosition((centreX - width * 0.5f) * unitsPerPixel,
                                    (height * 0.5f - centreY) * unitsPerPixel, -hud::kButtonDistance);
        buttonNode_[i]->setScale(scale, scale, scale);
    }

    const float labelRight = width - hud::kButtonMargin - 2.0f * hud::kButtonSize - hud::kGap - 10.0f;

    SceneNode* const rowLabels[6] = {zoomLabelNode_,    speedLabelNode_,  stepLabelNode_,
                                     restartLabelNode_, exportLabelNode_, saveLabelNode_};

    for (int row = 0; row < 6; ++row) {
        float centreX;
        float centreY;
        hudButtonCentre(row == 5   ? 8
                        : row == 4 ? 7
                        : row == 3 ? 6
                                   : row * 2,
                        width, height, hud::kButtonSize, hud::kButtonMargin, hud::kGap, centreX, centreY);

        SceneNode* node = rowLabels[row];
        if (node) {
            node->setPosition((labelRight - width * 0.5f) * unitsPerPixel,
                              (height * 0.5f - centreY) * unitsPerPixel, -hud::kButtonDistance);
            node->setScale(unitsPerPixel, unitsPerPixel, unitsPerPixel);
        }
    }

    if (speedLabel_) {
        char text[32];
        std::snprintf(text, sizeof(text), "speed x%g", speed);

        if (speedLabelShown_ != text) {
            speedLabelShown_ = text;
            setLabel(speedLabel_, speedLabelShown_);
        }
    }

    if (stepLabel_) {
        char text[48];
        std::snprintf(text, sizeof(text), "step %d/%d", shownEvents, totalEvents);

        if (stepLabelShown_ != text) {
            stepLabelShown_ = text;
            setLabel(stepLabel_, stepLabelShown_);
        }
    }
}

int ControlButtons::buttonAt(const HudContext& context, int x, int y) const
{
    float width = 0.0f;
    float height = 0.0f;
    if (!measureViewport(context, width, height)) {
        return None;
    }

    for (int i = 0; i < hud::kControlButtons; ++i) {
        float centreX;
        float centreY;
        hudButtonCentre(i, width, height, hud::kButtonSize, hud::kButtonMargin, hud::kGap, centreX, centreY);

        const float half = hud::kButtonSize * 0.5f;
        if (x >= centreX - half && x <= centreX + half && y >= centreY - half && y <= centreY + half) {
            return i;
        }
    }

    return None;
}
