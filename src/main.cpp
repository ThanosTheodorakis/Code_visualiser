// main.cpp - Program entry: starts the Ogre window and runs the demo or user program live, or replays a run.

#include "Renderer.h"
#include "RendererHandler.h"
#include "Markers.h"
#include "LiveSession.h"
#include "DemoProgram.h"
#include "ProjectFolders.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define CV_GETCWD _getcwd
#else
#include <unistd.h>
#define CV_GETCWD getcwd
#endif

#ifdef FV_USER_PROGRAM
int fvUserProgram();
#endif

namespace {
void runProgram()
{
#ifdef FV_USER_PROGRAM
    fvUserProgram();
#else
    demo::runDemo();
#endif
}

void reportEnvironment()
{
    char cwd[1024] = {0};
    if (CV_GETCWD(cwd, sizeof(cwd))) {
        std::cout << "[main] working directory: " << cwd << "\n";
    }
    for (const char* file : {"plugins.cfg", "resources.cfg"}) {
        std::ifstream in(file);
        std::cout << "[main]   " << file << ": " << (in.good() ? "found" : "NOT FOUND here") << "\n";
    }
    const std::string config = projectfolders::findConfig("config.cfg");
    std::ifstream configFile(config);
    std::cout << "[main]   config.cfg: "
              << (configFile.good() ? "found (" + config + ")" : "NOT FOUND, using defaults") << "\n";
}
}

int main(int argc, char** argv)
{
    float speed = 1.0f;
    int limit = 0;
    bool quiet = false;
    std::string replayPath;
    std::string savePath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--speed" && i + 1 < argc) {
            speed = static_cast<float>(std::atof(argv[++i]));
        } else if (arg == "--limit" && i + 1 < argc) {
            limit = std::atoi(argv[++i]);
        } else if (arg == "--quiet") {
            quiet = true;
        } else if (arg == "--replay" && i + 1 < argc) {
            replayPath = argv[++i];
        } else if (arg == "--save" && i + 1 < argc) {
            savePath = argv[++i];
        } else {
            std::cout
                << "usage: Code_visualiser [--speed N] [--limit N] [--quiet]\n"
                   "                          [--save FILE | --replay FILE]\n"
                   "  --speed N   starting playback speed (default 1)\n"
                   "  --limit N   how far the program may run ahead of the animation,\n"
                   "              in queued animations (default 8; 1 = nearly lockstep)\n"
                   "  --quiet     do not print every event to the console\n"
                   "  --save FILE    write the recorded run to FILE when the window is closed\n"
                   "  --replay FILE  play a saved run without running the program\n"
                   "  (while running: key S or the save button also saves the run to the runs folder of the project)\n";
            return arg == "--help" || arg == "-h" ? 0 : 2;
        }
    }

    reportEnvironment();

    try {
        Renderer& app = Renderer::getInstance();

        std::cout << "[main] initialising Ogre...\n";
        app.initApp();
        std::cout << "[main] Ogre initialised\n";

        app.gate().setSpeed(speed);
        if (limit > 0) {
            app.gate().setLimit(limit);
        }

        std::unique_ptr<LiveSession> session;

        if (!replayPath.empty()) {
            if (!app.loadRun(replayPath)) {
                app.closeApp();
                return 1;
            }
        } else {
            installRendererHandler(app);
            if (!quiet) {
                Trackers::installConsoleHandler();
            }

            session.reset(new LiveSession(runProgram));
            session->start();
        }

        std::cout << "[main] render loop running (ESC or close the window to quit)\n";
        app.getRoot()->startRendering();
        std::cout << "[main] render loop ended\n";

        if (session) {
            session->stopAndJoin();
        }
        if (!savePath.empty() && replayPath.empty()) {
            app.saveRun(savePath);
        }
        app.closeApp();
    } catch (const Ogre::Exception& e) {
        std::cerr << "[main] Ogre exception: " << e.getFullDescription() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "[main] error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
