// CameraController.cpp - Implementation of CameraController.

#include "CameraController.h"

#include <algorithm>
#include <cmath>
#include <iostream>

using namespace Ogre;

void CameraController::attach(Camera* camera, SceneNode* node, RenderWindow* window)
{
    camera_ = camera;
    node_ = node;
    window_ = window;
}

void CameraController::placeAtStart(const Vector3& chartStart)
{
    node_->setPosition(chartStart.x, chartStart.y - yOffset_, distanceStart_);
}

float CameraController::worldUnitsPerPixel() const
{
    if (!camera_ || !window_) {
        return 1.0f;
    }

    Viewport* viewport = window_->getViewport(0);

    if (!viewport) {
        return 1.0f;
    }

    float windowHeight = static_cast<float>(viewport->getActualHeight());

    if (windowHeight <= 0.0f) {
        return 1.0f;
    }

    Radian fovY = camera_->getFOVy();
    float distance = node_ ? std::abs(node_->getPosition().z) : distanceStart_;

    float viewHeight = 2.0f * tan(fovY.valueRadians() * 0.5f) * distance;

    return viewHeight / windowHeight;
}

void CameraController::zoomBy(float distanceFactor)
{
    if (!node_) {
        return;
    }

    Vector3 position = node_->getPosition();
    position.z = std::min(kZoomMax, std::max(kZoomMin, position.z * distanceFactor));
    node_->setPosition(position);
}

void CameraController::zoomWheel(int steps)
{
    zoomBy(std::pow(kZoomStep, static_cast<float>(steps)));
    touch();
}

void CameraController::scrollWheel(int steps)
{
    float pixelsToWorld = worldUnitsPerPixel();
    float amount = steps * pixelsToWorld * scrollMultiplier_;

    Vector3 position = node_->getPosition();
    position.y += amount;

    node_->setPosition(position);
    touch();
}

bool CameraController::startDrag(int button)
{
    if (dragging_) {
        return false;
    }

    dragging_ = true;
    dragButton_ = button;
    touch();

    enableAutoScroll(false);
    return true;
}

bool CameraController::endDrag(int button)
{
    if (!dragging_ || button != dragButton_) {
        return false;
    }

    dragging_ = false;
    touch();
    return true;
}

void CameraController::dragBy(int xrel, int yrel)
{
    float pixelsToWorld = worldUnitsPerPixel();

    float deltaX = -static_cast<float>(xrel) * pixelsToWorld;

    float deltaY = static_cast<float>(yrel) * pixelsToWorld;

    Vector3 position = node_->getPosition();
    position.x += deltaX;
    position.y += deltaY;

    node_->setPosition(position);
    touch();
}

void CameraController::followLatest(bool announce)
{
    autoScrollEnabled_ = true;

    lastManualInteraction_ =
        std::chrono::steady_clock::now() -
        std::chrono::milliseconds(static_cast<long long>(autoScrollTimeoutSeconds_ * 1000.0f) + 1);
    if (announce) {
        std::cout << "[camera] following the latest shape\n";
    }
}

void CameraController::enableAutoScroll(bool enable)
{
    autoScrollEnabled_ = enable;
    touch();
}

void CameraController::centreOn(const Vector3& cursor)
{
    if (node_) {
        Vector3 position = node_->getPosition();
        position.x = cursor.x;
        position.y = cursor.y;
        node_->setPosition(position);
    }
}

void CameraController::update(const Vector3& currentCursor)
{
    if (!node_ || !camera_) {
        return;
    }

    bool shouldAutoScroll = false;

    if (autoScrollEnabled_) {
        auto now = std::chrono::steady_clock::now();

        auto elapsed =
            std::chrono::duration_cast<std::chrono::duration<float>>(now - lastManualInteraction_).count();

        if (elapsed >= autoScrollTimeoutSeconds_ && !dragging_) {
            shouldAutoScroll = true;
        }
    }

    if (!shouldAutoScroll) {
        return;
    }

    Vector3 cameraPosition = node_->getPosition();

    const float zoomScale = std::min(1.0f, cameraPosition.z / distanceStart_);

    const float margin = scrollMargin_ * zoomScale;

    float deltaY = 0.0f;

    if (currentCursor.y > cameraPosition.y + margin) {
        deltaY = currentCursor.y - (cameraPosition.y + margin);
    } else if (currentCursor.y < cameraPosition.y - margin) {
        deltaY = currentCursor.y - (cameraPosition.y - margin);
    }

    float deltaX = 0.0f;

    if (std::abs(currentCursor.x - cameraPosition.x) > (layout_.xSpacing * 0.4f * zoomScale)) {
        deltaX = currentCursor.x - cameraPosition.x;
    }

    if (deltaX != 0.0f || deltaY != 0.0f) {
        const float interpolation = 0.12f;

        float newX = cameraPosition.x + deltaX * interpolation;

        float newY = cameraPosition.y + deltaY * interpolation;

        node_->setPosition(newX, newY, cameraPosition.z);
    }
}
