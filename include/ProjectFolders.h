// ProjectFolders.h - Locations of the runs and screenshots folders of the project.

#pragma once

#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

// Locations of the project folders.
namespace projectfolders {

// The project folder (set by CMake, else the working directory).
inline std::filesystem::path root()
{
#ifdef CV_PROJECT_DIR
    return std::filesystem::path(CV_PROJECT_DIR);
#else
    return std::filesystem::path(".");
#endif
}

// Folder for screenshots.
inline std::filesystem::path screenshots()
{
    return root() / "screenshots";
}

// Folder for saved runs.
inline std::filesystem::path runs()
{
    return root() / "runs";
}

// Path of the file called name in the first of the folders that has it; name itself if none has it.
inline std::string locate(const std::string& name, const std::vector<std::filesystem::path>& folders)
{
    for (const std::filesystem::path& folder : folders) {
        std::error_code ignored;
        const std::filesystem::path candidate = folder / name;
        if (std::filesystem::exists(candidate, ignored)) {
            return candidate.string();
        }
    }
    return name;
}

// Finds a settings file such as config.cfg: in the working directory, then in the app folder of the
// project, then in the project folder, so the program works from any directory.
inline std::string findConfig(const std::string& name)
{
    return locate(name, {std::filesystem::path("."), root() / "app", root()});
}

// Local time as YYYYMMDD_HHMMSS for file names.
inline std::string timestamp()
{
    char stamp[32];
    const std::time_t now = std::time(nullptr);
    std::tm local;
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &local);
    return stamp;
}

}
