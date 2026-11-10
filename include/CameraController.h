// CameraController.h - Scroll, zoom, drag and follow behaviour of the camera.

#pragma once

#include <chrono>

#include "Anim.h"
#include "Ogre.h"

// Scroll, zoom, drag and follow behaviour of the camera.
class CameraController
{
public:
    // Distance factor of one zoom step.
    static constexpr float kZoomStep = 0.8f;

    explicit CameraController(const LayoutParams& layout) : layout_(layout) {}

    // Connects the controller to the camera, its node and the window.
    void attach(Ogre::Camera* camera, Ogre::SceneNode* node, Ogre::RenderWindow* window);

    // True once attach has been called.
    bool attached() const { return node_ != nullptr; }

    // Puts the camera in front of the start of the chart.
    void placeAtStart(const Ogre::Vector3& chartStart);

    // Size of one screen pixel in world units at the current zoom.
    float worldUnitsPerPixel() const;

    // Moves the camera closer or further by a factor, within the zoom limits.
    void zoomBy(float distanceFactor);
    // Zooms by mouse wheel steps.
    void zoomWheel(int steps);
    // Scrolls the view up or down by mouse wheel steps.
    void scrollWheel(int steps);

    // Starts dragging the view; returns false if a drag is already active.
    bool startDrag(int button);
    // Ends the drag started with the same button.
    bool endDrag(int button);
    // Moves the view by a mouse movement in pixels.
    void dragBy(int xrel, int yrel);

    // True while the view is being dragged.
    bool dragging() const { return dragging_; }

    // Resumes following the newest shape.
    void followLatest(bool announce = true);
    // Turns automatic following on or off.
    void enableAutoScroll(bool enable);

    // Eases the camera toward the cursor when following is active.
    void update(const Ogre::Vector3& cursor);

    // Jumps the camera to the cursor.
    void centreOn(const Ogre::Vector3& cursor);

private:
    // Notes a manual interaction, which pauses following for a moment.
    void touch() { lastManualInteraction_ = std::chrono::steady_clock::now(); }

    const LayoutParams& layout_;
    Ogre::Camera* camera_ = nullptr;
    Ogre::SceneNode* node_ = nullptr;
    Ogre::RenderWindow* window_ = nullptr;

    float distanceStart_ = 380.0f;
    float yOffset_ = 70.0f;
    float scrollMargin_ = 120.0f;
    float scrollMultiplier_ = 50.0f;
    float autoScrollTimeoutSeconds_ = 2.0f;
    bool autoScrollEnabled_ = true;

    bool dragging_ = false;
    int dragButton_ = 0;
    std::chrono::steady_clock::time_point lastManualInteraction_ = std::chrono::steady_clock::now();

    static constexpr float kZoomMin = 40.0f;
    static constexpr float kZoomMax = 900.0f;
};
