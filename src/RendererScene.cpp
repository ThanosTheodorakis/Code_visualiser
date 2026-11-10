// RendererScene.cpp - Renderer part that draws the shapes of the chart as Ogre objects.

#include "Renderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "ShapeGeometry.h"
#include "StrokeFont.h"

using namespace Ogre;

float Renderer::nextLayerZ()
{
    return 0.012f * static_cast<float>(layerCounter++ % 40);
}

void Renderer::ensureAnimObject(Anim& animation, const char* prefix)
{
    if (animation.obj) {
        return;
    }

    animation.z = nextLayerZ();
    animation.obj = scnMgr->createManualObject(std::string(prefix) + std::to_string(ogreHeadCount++));
    animation.obj->setDynamic(true);

    attachToWorld(animation.obj);
}

void Renderer::attachToWorld(ManualObject* object)
{
    worldNode->createChildSceneNode()->attachObject(object);
    worldObjects.push_back(object);
}

void Renderer::drawFlow(Anim& animation, bool settled)
{
    if (!scnMgr) {
        return;
    }

    ensureAnimObject(animation, "AnimFlow_");

    const bool appearsWhole = !animation.advanceCursorOnFinish;

    const float progress = appearsWhole ? 1.0f : std::min(1.0f, animation.elapsed / animation.duration);

    const Vector3 tipPosition = animation.startPos + (animation.endPos - animation.startPos) * progress;

    animation.tip = tipPosition;

    noteExtent(std::min(animation.startPos.x, tipPosition.x), std::min(animation.startPos.y, tipPosition.y),
               std::max(animation.startPos.x, tipPosition.x), std::max(animation.startPos.y, tipPosition.y));

    const ColourValue colour =
        settled ? shapes::scaled(animation.color, 0.8f) : shapes::towardWhite(animation.color, 0.35f);

    const float width = settled ? lineWidth : activeLineWidth;
    const float z = animation.z + (settled ? 0.0f : 0.6f);

    const float dx = tipPosition.x - animation.startPos.x;
    const float dy = tipPosition.y - animation.startPos.y;
    const float length = std::sqrt(dx * dx + dy * dy);

    animation.obj->clear();
    animation.obj->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    if (animation.path.size() >= 2) {
        animation.tip = animation.path.back();

        for (std::size_t i = 0; i + 1 < animation.path.size(); ++i) {
            const Vector3& a = animation.path[i];
            const Vector3& b = animation.path[i + 1];

            shapes::dashedLine(animation.obj, index, a.x, a.y, b.x, b.y, width, dashLength, dashGap, z,
                               colour);
        }

        const Vector3& before = animation.path[animation.path.size() - 2];
        const Vector3& last = animation.path.back();

        shapes::arrowHead(animation.obj, index, last.x, last.y, last.x - before.x, last.y - before.y,
                          arrowSize, z, colour);

        shapes::ensureNotEmpty(animation.obj, index, colour);
        animation.obj->end();
        return;
    }

    if (animation.dashed) {
        shapes::dashedLine(animation.obj, index, animation.startPos.x, animation.startPos.y, tipPosition.x,
                           tipPosition.y, width, dashLength, dashGap, z, colour);
    } else {
        shapes::thickLine(animation.obj, index, animation.startPos.x, animation.startPos.y, tipPosition.x,
                          tipPosition.y, width, z, colour);
    }

    if (length >= arrowSize * 1.6f) {
        shapes::arrowHead(animation.obj, index, tipPosition.x, tipPosition.y, dx, dy, arrowSize, z, colour);
    }

    shapes::ensureNotEmpty(animation.obj, index, colour);
    animation.obj->end();
}

void Renderer::drawLoop(Anim& animation, float progress, bool settled)
{
    if (!scnMgr) {
        return;
    }

    ensureAnimObject(animation, "AnimLoop_");

    const float startAngle = Ogre::Math::HALF_PI;
    const float sweep = Ogre::Math::TWO_PI * progress;
    const float endAngle = startAngle + sweep;

    const ColourValue colour =
        settled ? shapes::scaled(animation.color, 0.8f) : shapes::towardWhite(animation.color, 0.35f);

    const float width = settled ? lineWidth : activeLineWidth;
    const float z = animation.z + (settled ? 0.0f : 0.6f);

    const float tipX = animation.center.x + animation.radiusX * std::cos(endAngle);
    const float tipY = animation.center.y + animation.radiusY * std::sin(endAngle);

    animation.tip = Vector3(tipX, tipY, 0);

    noteExtent(animation.center.x - animation.radiusX, animation.center.y - animation.radiusY,
               animation.center.x + animation.radiusX, animation.center.y + animation.radiusY);

    animation.obj->clear();
    animation.obj->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    shapes::ellipseFan(animation.obj, index, animation.center.x, animation.center.y, animation.radiusX,
                       animation.radiusY, startAngle, endAngle, animation.z - 2.0f,
                       shapes::scaled(animation.color, 0.22f));

    shapes::ellipseBand(animation.obj, index, animation.center.x, animation.center.y, animation.radiusX,
                        animation.radiusY, startAngle, endAngle, width, z, colour);

    if (sweep > 0.4f) {
        shapes::arrowHead(animation.obj, index, tipX, tipY, -animation.radiusX * std::sin(endAngle),
                          animation.radiusY * std::cos(endAngle), arrowSize, z, colour);
    }

    shapes::ensureNotEmpty(animation.obj, index, colour);
    animation.obj->end();
}

void Renderer::drawIO(Anim& animation, float progress, bool settled, bool waiting)
{
    if (!scnMgr) {
        return;
    }

    ensureAnimObject(animation, "AnimIO_");

    const float cx = animation.center.x;
    const float cy = animation.center.y;
    const float w = animation.radiusX;
    const float h = animation.radiusY;

    noteExtent(cx - w, cy - h, cx + w, cy + h);

    std::vector<float> px;
    std::vector<float> py;

    px.push_back(cx);
    py.push_back(cy + h);

    if (animation.terminator) {
        const float r = h;
        const int steps = 12;

        px.push_back(cx - w + r);
        py.push_back(cy + h);

        for (int i = 1; i < steps; ++i) {
            const float a = shapes::kPi * 0.5f + shapes::kPi * static_cast<float>(i) / steps;
            px.push_back(cx - w + r + r * std::cos(a));
            py.push_back(cy + r * std::sin(a));
        }

        px.push_back(cx - w + r);
        py.push_back(cy - h);
        px.push_back(cx + w - r);
        py.push_back(cy - h);

        for (int i = 1; i < steps; ++i) {
            const float a = -shapes::kPi * 0.5f + shapes::kPi * static_cast<float>(i) / steps;
            px.push_back(cx + w - r + r * std::cos(a));
            py.push_back(cy + r * std::sin(a));
        }

        px.push_back(cx + w - r);
        py.push_back(cy + h);
    } else {
        const float skew = h * 1.1f;

        px.push_back(cx - w + 2.0f * skew);
        py.push_back(cy + h);
        px.push_back(cx - w);
        py.push_back(cy - h);
        px.push_back(cx + w - 2.0f * skew);
        py.push_back(cy - h);
        px.push_back(cx + w);
        py.push_back(cy + h);
    }

    px.push_back(cx);
    py.push_back(cy + h);

    const int segmentCount = static_cast<int>(px.size()) - 1;

    std::vector<float> lengths(segmentCount);
    float total = 0.0f;
    for (int i = 0; i < segmentCount; ++i) {
        lengths[i] =
            std::sqrt((px[i + 1] - px[i]) * (px[i + 1] - px[i]) + (py[i + 1] - py[i]) * (py[i + 1] - py[i]));
        total += lengths[i];
    }

    ColourValue colour =
        settled ? shapes::scaled(animation.color, 0.8f) : shapes::towardWhite(animation.color, 0.35f);

    if (waiting) {
        const double seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(seconds * 5.0));
        colour = shapes::towardWhite(animation.color, 0.15f + 0.6f * pulse);
    }

    const float width = settled ? lineWidth : activeLineWidth;
    const float z = animation.z + (settled ? 0.0f : 0.6f);
    const ColourValue fill = shapes::scaled(animation.color, 0.22f);

    animation.obj->clear();
    animation.obj->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    float remaining = std::min(1.0f, std::max(0.0f, progress)) * total;
    float tipX = px[0];
    float tipY = py[0];
    float dirX = -1.0f;
    float dirY = 0.0f;

    for (int i = 0; i < segmentCount && remaining > 0.0f; ++i) {
        const float used = std::min(remaining, lengths[i]);
        const float t = lengths[i] > 0.0f ? used / lengths[i] : 1.0f;
        const float ex = px[i] + (px[i + 1] - px[i]) * t;
        const float ey = py[i] + (py[i + 1] - py[i]) * t;

        shapes::triangleFill(animation.obj, index, cx, cy, px[i], py[i], ex, ey, animation.z - 2.0f, fill);

        shapes::thickLine(animation.obj, index, px[i], py[i], ex, ey, width, z, colour);

        tipX = ex;
        tipY = ey;
        dirX = px[i + 1] - px[i];
        dirY = py[i + 1] - py[i];
        remaining -= used;
    }

    animation.tip = Vector3(tipX, tipY, 0);

    if (progress > 0.1f) {
        shapes::arrowHead(animation.obj, index, tipX, tipY, dirX, dirY, arrowSize, z, colour);
    }

    shapes::ensureNotEmpty(animation.obj, index, colour);
    animation.obj->end();
}

void Renderer::drawFork(Anim& animation, int mode, float progress)
{
    if (!scnMgr) {
        return;
    }

    ensureAnimObject(animation, "AnimFork_");

    const bool finished = (mode == 2);

    const ColourValue colour =
        finished ? shapes::scaled(animation.color, 0.8f) : shapes::towardWhite(animation.color, 0.35f);

    const float width = finished ? lineWidth : activeLineWidth;
    const float z = animation.z + (finished ? 0.0f : 0.6f);

    animation.obj->clear();
    animation.obj->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    const Vector3& start = animation.startPos;
    const Vector3& junction = animation.endPos;

    if (mode == 0) {
        const Vector3 current = start + (junction - start) * progress;
        animation.tip = current;

        noteExtent(std::min(start.x, current.x), std::min(start.y, current.y), std::max(start.x, current.x),
                   std::max(start.y, current.y));

        shapes::thickLine(animation.obj, index, start.x, start.y, current.x, current.y, width, z, colour);

        const float dx = current.x - start.x;
        const float dy = current.y - start.y;
        if (std::sqrt(dx * dx + dy * dy) >= arrowSize * 1.6f) {
            shapes::arrowHead(animation.obj, index, current.x, current.y, dx, dy, arrowSize, z, colour);
        }
    } else {
        animation.tip = junction;

        const Vector3 left = junction + (animation.branchLeftTarget - junction) * progress;
        const Vector3 right = junction + (animation.branchRightTarget - junction) * progress;

        noteExtent(std::min(left.x, right.x), std::min(left.y, right.y), std::max(left.x, right.x),
                   std::max(junction.y, left.y));

        shapes::triangleFill(animation.obj, index, junction.x, junction.y, left.x, left.y, right.x, right.y,
                             animation.z - 2.0f, shapes::scaled(animation.color, 0.22f));

        shapes::thickLine(animation.obj, index, start.x, start.y, junction.x, junction.y, width, z, colour);

        if (!finished) {
            shapes::thickLine(animation.obj, index, junction.x, junction.y, left.x, left.y, width, z, colour);
            shapes::thickLine(animation.obj, index, junction.x, junction.y, right.x, right.y, width, z,
                              colour);
        } else {
            const Vector3& notTaken =
                animation.takenRight ? animation.branchLeftTarget : animation.branchRightTarget;

            shapes::dashedLine(animation.obj, index, junction.x, junction.y, notTaken.x, notTaken.y,
                               lineWidth, dashLength, dashGap, z, shapes::scaled(animation.color, 0.45f));
        }

        shapes::diamond(animation.obj, index, junction.x, junction.y, width * 1.9f, z + 0.1f, colour);
    }

    shapes::ensureNotEmpty(animation.obj, index, colour);
    animation.obj->end();
}

void Renderer::createForkArmLabels(const Anim& animation)
{
    const float height = 8.0f * layout.drawScale;
    const float gap = 9.0f * layout.drawScale;

    const Vector3& junction = animation.endPos;

    const Vector3 leftMid((junction.x + animation.branchLeftTarget.x) * 0.5f,
                          (junction.y + animation.branchLeftTarget.y) * 0.5f, 0);

    const Vector3 rightMid((junction.x + animation.branchRightTarget.x) * 0.5f,
                           (junction.y + animation.branchRightTarget.y) * 0.5f, 0);

    createTextObject(animation.armLeft.empty() ? std::string("false") : animation.armLeft, leftMid.x - gap,
                     leftMid.y - height * 0.5f, height, animation.color, true);

    createTextObject(animation.armRight.empty() ? std::string("true") : animation.armRight, rightMid.x + gap,
                     rightMid.y - height * 0.5f, height, animation.color, false);
}

void Renderer::updateCursorMarker()
{
    if (!scnMgr) {
        return;
    }

    if (!cursorMarker) {
        cursorMarker = scnMgr->createManualObject("FvCursorMarker");
        cursorMarker->setDynamic(true);
        attachToWorld(cursorMarker);
    }

    Vector3 position;

    if (!activeAnims.empty()) {
        position = activeAnims.front().tip;
    } else {
        std::lock_guard<std::mutex> lock(sceneMutex);
        position = cursor;
    }

    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();

    const float radius = 2.6f * (1.0f + 0.25f * static_cast<float>(std::sin(seconds * 6.0)));

    cursorMarker->clear();
    cursorMarker->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    shapes::diamond(cursorMarker, index, position.x, position.y, radius, 3.0f,
                    configuration.getConfig().cursorColor);

    shapes::diamond(cursorMarker, index, position.x, position.y, radius * 0.5f, 3.1f,
                    ColourValue(0.05f, 0.05f, 0.08f));

    cursorMarker->end();
}

ManualObject* Renderer::createLabelForAnim(const Anim& animation, bool atStart,
                                           const std::string& textOverride)
{
    const std::string& text =
        !textOverride.empty() ? textOverride : (atStart ? animation.startText : animation.endText);

    if (text.empty() || !scnMgr) {
        return nullptr;
    }

    float rightX;
    float topY;
    float bottomY;

    if (animation.type == AnimType::LOOP || animation.type == AnimType::IO) {
        rightX = animation.center.x + animation.radiusX;
        topY = animation.center.y + animation.radiusY;
        bottomY = animation.center.y - animation.radiusY;
    } else {
        rightX = std::max(animation.startPos.x, animation.endPos.x);
        topY = std::max(animation.startPos.y, animation.endPos.y);
        bottomY = std::min(animation.startPos.y, animation.endPos.y);
    }

    const float textHeight = 10.0f * layout.drawScale;
    const float gap = 8.0f * layout.drawScale;

    const float margin = textHeight * 0.25f;

    const float originX = rightX + gap;
    const float lift = (!atStart && animation.dashed) ? arrowSize : 0.0f;
    const float originY = atStart ? topY - margin - textHeight : bottomY + margin + lift;

    return createTextObject(text, originX, originY, textHeight, animation.color, false, textOverride.empty());
}

ManualObject* Renderer::createTextObject(const std::string& text, float x, float y, float height,
                                         const ColourValue& base, bool alignRight, bool countExtent)
{
    if (text.empty() || !scnMgr) {
        return nullptr;
    }

    const std::vector<strokefont::Segment> segments = strokefont::layout(text, height);

    if (segments.empty()) {
        return nullptr;
    }

    float minX = segments[0].x1;
    float maxX = segments[0].x1;
    float minY = segments[0].y1;
    float maxY = segments[0].y1;

    for (const strokefont::Segment& segment : segments) {
        minX = std::min(minX, std::min(segment.x1, segment.x2));
        maxX = std::max(maxX, std::max(segment.x1, segment.x2));
        minY = std::min(minY, std::min(segment.y1, segment.y2));
        maxY = std::max(maxY, std::max(segment.y1, segment.y2));
    }

    const float originX = alignRight ? x - maxX : x;
    const float pad = height * 0.2f;

    if (countExtent) {
        noteExtent(originX + minX - pad, y + minY - pad, originX + maxX + pad, y + maxY + pad);
    }
    const float strokeWidth = height * 0.14f;
    const ColourValue textColour = shapes::towardWhite(base, 0.45f);
    const ColourValue plateColour = shapes::scaled(base, 0.16f);

    ManualObject* object = scnMgr->createManualObject("Label_" + std::to_string(ogreHeadCount++));

    object->setDynamic(false);
    object->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    shapes::rect(object, index, originX + minX - pad, y + minY - pad, originX + maxX + pad, y + maxY + pad,
                 -0.5f, plateColour);

    for (const strokefont::Segment& segment : segments) {
        shapes::thickLine(object, index, originX + segment.x1, y + segment.y1, originX + segment.x2,
                          y + segment.y2, strokeWidth, 0.6f, textColour, true);
    }

    shapes::ensureNotEmpty(object, index, textColour);
    object->end();

    attachToWorld(object);

    return object;
}

void Renderer::openFunctionBox(const std::string& name, const Vector3& at)
{
    if (!scnMgr) {
        return;
    }

    ManualObject* object = scnMgr->createManualObject("FunctionBox_" + std::to_string(ogreHeadCount++));
    object->setDynamic(true);

    attachToWorld(object);

    boxes.open(name, at, object);
}

void Renderer::closeFunctionBox(float bottomY, float returnX)
{
    FunctionBox* box = boxes.close(bottomY, returnX);
    if (box) {
        drawFunctionBox(*box);
    }
}

void Renderer::noteExtent(float x0, float y0, float x1, float y1)
{
    boxes.noteExtent(x0, y0, x1, y1);
}

void Renderer::updateFunctionBoxes()
{
    for (FunctionBox* box : boxes.boxesToRedraw()) {
        drawFunctionBox(*box);
    }
}

void Renderer::drawFunctionBox(FunctionBox& box)
{
    box.dirty = false;

    if (!box.obj) {
        return;
    }

    const ConfigData& config = configuration.getConfig();

    const int level = std::min(box.depth, 8);
    const int shade = box.depth % 4;

    const float left = box.minX - boxes.padding();
    const float right = box.maxX + boxes.padding();
    const float top = box.topY;
    const float bottom = box.closed ? box.bottomY : box.minY - boxes.padding();

    const ColourValue fill = shapes::scaled(config.functionLineColor, 0.10f + 0.02f * shade);

    const ColourValue frame =
        shapes::scaled(shapes::towardWhite(config.functionLineColor, 0.12f * shade), 0.65f);

    const float zFill = -5.0f + 0.25f * level;
    const float zFrame = -0.3f + 0.02f * level;
    const float edge = 0.7f;

    box.obj->clear();
    box.obj->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);

    int index = 0;

    if (top > bottom && right > left) {
        shapes::rect(box.obj, index, left, bottom, right, top, zFill, fill);

        shapes::rect(box.obj, index, left, bottom, right, bottom + edge, zFrame, frame);
        shapes::rect(box.obj, index, left, top - edge, right, top, zFrame, frame);
        shapes::rect(box.obj, index, left, bottom, left + edge, top, zFrame, frame);
        shapes::rect(box.obj, index, right - edge, bottom, right, top, zFrame, frame);
    }

    shapes::ensureNotEmpty(box.obj, index, frame);
    box.obj->end();
}
