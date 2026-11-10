// ImageExporter.cpp - Implementation of ImageExporter.

#include "ImageExporter.h"

#include <filesystem>
#include <iostream>
#include <string>

#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTexture.h>
#include <OgreTextureManager.h>

#include "ProjectFolders.h"

using namespace Ogre;

void ImageExporter::attach(SceneManager* sceneManager, SceneNode* interfaceNode, RenderWindow* window,
                           Camera* camera)
{
    scnMgr_ = sceneManager;
    interfaceNode_ = interfaceNode;
    window_ = window;
    camera_ = camera;
}

void ImageExporter::exportView(ManualObject* cursorMarker, const ColourValue& background)
{
    SceneManager* scnMgr = scnMgr_;
    SceneNode* camNode = interfaceNode_;
    if (!scnMgr || !camNode || !window_ || !camera_) {
        return;
    }

    const int pixelW = static_cast<int>(window_->getWidth());
    const int pixelH = static_cast<int>(window_->getHeight());
    if (pixelW < 1 || pixelH < 1) {
        return;
    }

    const std::string stamp = projectfolders::timestamp();

    const std::filesystem::path screenshotDir = projectfolders::screenshots();
    std::error_code folderError;
    std::filesystem::create_directories(screenshotDir, folderError);
    const std::string fileName = (screenshotDir / (std::string("view_") + stamp + ".png")).string();

    TexturePtr texture;
    RenderTexture* target = nullptr;

    try {
        texture = TextureManager::getSingleton().createManual(
            "FvExportTexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, pixelW, pixelH,
            0, PF_BYTE_RGBA, TU_RENDERTARGET);

        target = texture->getBuffer()->getRenderTarget();
        target->setAutoUpdated(false);

        Viewport* view = target->addViewport(camera_);
        view->setClearEveryFrame(true);
        view->setBackgroundColour(background);
        view->setOverlaysEnabled(false);

        Viewport* main = window_->getViewport(0);
        if (main) {
            view->setMaterialScheme(main->getMaterialScheme());
        }

        const bool cursorWasVisible = cursorMarker && cursorMarker->getVisible();
        camNode->setVisible(false);
        if (cursorMarker) {
            cursorMarker->setVisible(false);
        }

        target->update();

        camNode->setVisible(true);
        if (cursorMarker) {
            cursorMarker->setVisible(cursorWasVisible);
        }

        target->writeContentsToFile(fileName);
        std::cout << "[export] saved " << std::filesystem::absolute(fileName).string() << " (" << pixelW
                  << "x" << pixelH << ")\n";
    } catch (const Ogre::Exception& e) {
        camNode->setVisible(true);
        std::cout << "[export] failed: " << e.getDescription() << "\n";
    }

    if (target) {
        target->removeAllViewports();
    }
    texture.reset();
    if (TextureManager::getSingleton().resourceExists("FvExportTexture")) {
        TextureManager::getSingleton().remove("FvExportTexture");
    }
}
