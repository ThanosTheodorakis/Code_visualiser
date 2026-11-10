// Anim.h - One animated piece of the chart and the spacing settings shared by the renderer.

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Ogre.h"

// Spacing and scale settings shared by the renderer, planner and camera.
struct LayoutParams
{
    float xSpacing = 140.0f;
    float ySpacing = 110.0f;
    float drawScale = 0.6f;
    float functionIndent = 0.5f;

    // Length of one plain downward step.
    float flowStep() const { return ySpacing * drawScale * 0.45f; }
};

// Kind of shape an animation draws.
enum class AnimType
{
    FLOW,
    LOOP,
    FORK,
    IO
};

// One animated piece of the chart: its shape, timing, text and the state it needs while playing.
struct Anim
{
    AnimType type = AnimType::FLOW;
    Ogre::ManualObject* obj = nullptr;

    float duration = 1.0f;
    float elapsed = 0.0f;
    Ogre::ColourValue color{1.0f, 0.3f, 0.1f};

    std::string label;
    bool showLabel = false;

    std::string startText;
    std::string endText;

    Ogre::Vector3 startPos;
    Ogre::Vector3 endPos;

    Ogre::Vector3 center;
    float radius = 0.0f;
    float radiusX = 0.0f;
    float radiusY = 0.0f;

    bool takenRight = false;
    std::string armLeft;
    std::string armRight;
    bool stageIncomingDone = false;
    Ogre::Vector3 branchLeftTarget;
    Ogre::Vector3 branchRightTarget;

    bool hasGeometry = false;

    int reconnectForkId = -1;
    int reconnectLoopId = -1;

    int forkId = -1;
    int loopId = -1;

    bool advanceCursorOnFinish = true;

    bool isFunctionExit = false;

    bool opensBox = false;
    bool closesBox = false;
    std::string boxName;

    bool dashed = false;
    std::vector<Ogre::Vector3> path;

    bool terminator = false;
    std::string centerText;
    bool ioWaits = false;
    Ogre::ManualObject* waitLabel = nullptr;
    float z = 0.0f;
    Ogre::Vector3 tip;
    std::size_t eventIndex = static_cast<std::size_t>(-1);
};

// Position and size of a loop's circle, so its return line can reconnect later.
struct LoopJunction
{
    int id;
    Ogre::Vector3 center;
    float radiusX;
    float radiusY;
};
