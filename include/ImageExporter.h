// ImageExporter.h - Saves what the camera currently sees, without the interface, as a PNG picture.

#pragma once

#include "Ogre.h"

// Saves the current view as a PNG file.
class ImageExporter
{
public:
    // Connects the exporter to the scene; the interface node is hidden while saving.
    void attach(Ogre::SceneManager* sceneManager, Ogre::SceneNode* interfaceNode, Ogre::RenderWindow* window,
                Ogre::Camera* camera);

    // Renders the camera view without the interface into the screenshots folder.
    void exportView(Ogre::ManualObject* cursor, const Ogre::ColourValue& background);

private:
    Ogre::SceneManager* scnMgr_ = nullptr;
    Ogre::SceneNode* interfaceNode_ = nullptr;
    Ogre::RenderWindow* window_ = nullptr;
    Ogre::Camera* camera_ = nullptr;
};
