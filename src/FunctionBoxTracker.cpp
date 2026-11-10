// FunctionBoxTracker.cpp - Implementation of FunctionBoxTracker.

#include "FunctionBoxTracker.h"

#include <algorithm>

void FunctionBoxTracker::open(const std::string& name, const Ogre::Vector3& at, Ogre::ManualObject* obj)
{
    FunctionBox box;
    box.name = name;
    box.depth = static_cast<int>(open_.size());
    box.topY = at.y;
    box.startX = at.x;
    box.minX = at.x;
    box.maxX = at.x;
    box.minY = at.y;
    box.obj = obj;

    boxes_.push_back(box);
    open_.push_back(static_cast<int>(boxes_.size()) - 1);
}

FunctionBox* FunctionBoxTracker::close(float bottomY, float returnX)
{
    if (open_.empty()) {
        return nullptr;
    }

    FunctionBox& box = boxes_[open_.back()];
    box.bottomY = std::min(bottomY, box.minY);
    box.returnX = returnX;
    box.closed = true;
    box.dirty = true;

    open_.pop_back();
    return &box;
}

void FunctionBoxTracker::noteExtent(float x0, float y0, float x1, float)
{
    for (std::size_t i = open_.size(); i-- > 0;) {
        FunctionBox& box = boxes_[open_[i]];

        if (x0 < box.minX) {
            box.minX = x0;
            box.dirty = true;
        }
        if (x1 > box.maxX) {
            box.maxX = x1;
            box.dirty = true;
        }
        if (y0 < box.minY) {
            box.minY = y0;
            box.dirty = true;
        }

        x0 -= pad_;
        x1 += pad_;
        y0 -= pad_;
    }
}

std::vector<FunctionBox*> FunctionBoxTracker::boxesToRedraw()
{
    std::vector<FunctionBox*> result;
    for (int index : open_) {
        if (boxes_[index].dirty) {
            result.push_back(&boxes_[index]);
        }
    }
    return result;
}

void FunctionBoxTracker::clear()
{
    boxes_.clear();
    open_.clear();
}
