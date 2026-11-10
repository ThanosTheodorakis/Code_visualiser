// FunctionBoxTracker.h - Tracks the size and nesting of the boxes drawn around function calls.

#pragma once

#include <deque>
#include <string>
#include <vector>

#include "Ogre.h"

// The rectangle drawn around one function call, with its bounds.
struct FunctionBox
{
    Ogre::ManualObject* obj = nullptr;
    std::string name;
    int depth = 0;
    float topY = 0.0f;
    float startX = 0.0f;
    float returnX = 0.0f;
    float minX = 0.0f;
    float maxX = 0.0f;
    float minY = 0.0f;
    float bottomY = 0.0f;
    bool closed = false;
    bool dirty = true;
};

// Tracks the nesting and growing size of the function boxes.
class FunctionBoxTracker
{
public:
    // Gap between nested boxes.
    float padding() const { return pad_; }

    // Starts a box at a position.
    void open(const std::string& name, const Ogre::Vector3& at, Ogre::ManualObject* obj);

    // Closes the innermost box and returns it, or nullptr if none is open.
    FunctionBox* close(float bottomY, float returnX);

    // Widens the open boxes so they contain the given area.
    void noteExtent(float x0, float y0, float x1, float y1);

    // The open boxes whose size changed.
    std::vector<FunctionBox*> boxesToRedraw();

    // True while a box is open.
    bool anyOpen() const { return !open_.empty(); }

    // Left edge of the innermost open box.
    float innermostMinX() const { return boxes_[open_.back()].minX; }

    // Forgets all boxes.
    void clear();

private:
    std::deque<FunctionBox> boxes_;
    std::vector<int> open_;
    float pad_ = 5.0f;
};
