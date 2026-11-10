# CMakeLists.txt - Builds the Code_visualiser application with Ogre and sets up the tests.

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

project(CodeVisualiser)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 AND NOT OGRE_DIR)
    set(OGRE_DIR "C:/Ogre/out/build/sdk/CMake" CACHE PATH "Folder with OGREConfig.cmake" FORCE)
endif()
find_package(OGRE REQUIRED COMPONENTS Bites CONFIG)
find_package(Threads REQUIRED)

if(NOT OGRE_SOURCE_ROOT)
    set(_ogre_cache "${OGRE_DIR}/../../CMakeCache.txt")
    if(EXISTS "${_ogre_cache}")
        file(STRINGS "${_ogre_cache}" _home REGEX "^CMAKE_HOME_DIRECTORY:INTERNAL=")
        if(_home)
            string(REGEX REPLACE "^CMAKE_HOME_DIRECTORY:INTERNAL=" "" _home "${_home}")
            set(OGRE_SOURCE_ROOT "${_home}" CACHE PATH "Ogre source folder (has Media/ and Samples/)" FORCE)
        endif()
    endif()
endif()
if(NOT OGRE_SOURCE_ROOT OR NOT EXISTS "${OGRE_SOURCE_ROOT}/Media/Main")
    message(WARNING "Could not find the Ogre source folder (the one with Media/). "
                    "Run CMake again with -DOGRE_SOURCE_ROOT=<path to your ogre folder>.")
    if(NOT OGRE_SOURCE_ROOT)
        set(OGRE_SOURCE_ROOT "C:/Ogre/ogre")
    endif()
endif()
message(STATUS "Ogre source folder: ${OGRE_SOURCE_ROOT}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/resources.cfg.in"
               "${CMAKE_CURRENT_SOURCE_DIR}/resources.cfg" @ONLY)

get_filename_component(CV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(CV_SOURCES
    ${CV_ROOT}/src/main.cpp
    ${CV_ROOT}/src/AnimPlanner.cpp
    ${CV_ROOT}/src/CallStackPanel.cpp
    ${CV_ROOT}/src/CallStackTracker.cpp
    ${CV_ROOT}/src/CameraController.cpp
    ${CV_ROOT}/src/ControlButtons.cpp
    ${CV_ROOT}/src/EventLog.cpp
    ${CV_ROOT}/src/FunctionBoxTracker.cpp
    ${CV_ROOT}/src/Hud.cpp
    ${CV_ROOT}/src/HudCommon.cpp
    ${CV_ROOT}/src/ImageExporter.cpp
    ${CV_ROOT}/src/InputController.cpp
    ${CV_ROOT}/src/Legend.cpp
    ${CV_ROOT}/src/Markers.cpp
    ${CV_ROOT}/src/PauseButton.cpp
    ${CV_ROOT}/src/RenderEvents.cpp
    ${CV_ROOT}/src/Renderer.cpp
    ${CV_ROOT}/src/RendererAnimation.cpp
    ${CV_ROOT}/src/RendererHandler.cpp
    ${CV_ROOT}/src/RendererScene.cpp
    ${CV_ROOT}/src/RunStore.cpp
    ${CV_ROOT}/src/StatusIndicator.cpp
    ${CV_ROOT}/src/configParser.cpp)

set(CV_HEADERS
    ${CV_ROOT}/include/Anim.h
    ${CV_ROOT}/include/AnimPlanner.h
    ${CV_ROOT}/include/Backlog.h
    ${CV_ROOT}/include/CallStackPanel.h
    ${CV_ROOT}/include/CallStackTracker.h
    ${CV_ROOT}/include/CameraController.h
    ${CV_ROOT}/include/ControlButtons.h
    ${CV_ROOT}/include/DemoProgram.h
    ${CV_ROOT}/include/EventLog.h
    ${CV_ROOT}/include/FunctionBoxTracker.h
    ${CV_ROOT}/include/Hud.h
    ${CV_ROOT}/include/HudCommon.h
    ${CV_ROOT}/include/ImageExporter.h
    ${CV_ROOT}/include/InputController.h
    ${CV_ROOT}/include/InterruptibleInput.h
    ${CV_ROOT}/include/Instrument.h
    ${CV_ROOT}/include/InstrumentEnd.h
    ${CV_ROOT}/include/Legend.h
    ${CV_ROOT}/include/LiveSession.h
    ${CV_ROOT}/include/Markers.h
    ${CV_ROOT}/include/PauseButton.h
    ${CV_ROOT}/include/PlayerControls.h
    ${CV_ROOT}/include/ProjectFolders.h
    ${CV_ROOT}/include/RenderEvents.h
    ${CV_ROOT}/include/Renderer.h
    ${CV_ROOT}/include/RendererHandler.h
    ${CV_ROOT}/include/RunFile.h
    ${CV_ROOT}/include/RunStore.h
    ${CV_ROOT}/include/ShapeGeometry.h
    ${CV_ROOT}/include/StatusIndicator.h
    ${CV_ROOT}/include/StrokeFont.h
    ${CV_ROOT}/include/configParser.h)

add_executable(Code_visualiser ${CV_SOURCES} ${CV_HEADERS})
target_include_directories(Code_visualiser PRIVATE ${CV_ROOT}/include)
get_filename_component(CV_PROJECT_DIR "${CV_ROOT}" ABSOLUTE)
target_compile_definitions(Code_visualiser PRIVATE CV_PROJECT_DIR="${CV_PROJECT_DIR}")

set(FV_USER_PROGRAM "" CACHE STRING "Instrumented source file(s) to visualise instead of the demo")
if(FV_USER_PROGRAM)
    set(FV_USER_SOURCES "")
    set(FV_USER_DIRS "")
    foreach(user_file ${FV_USER_PROGRAM})
        get_filename_component(user_abs "${user_file}" ABSOLUTE)
        get_filename_component(user_dir "${user_abs}" DIRECTORY)
        list(APPEND FV_USER_SOURCES "${user_abs}")
        list(APPEND FV_USER_DIRS "${user_dir}")
    endforeach()
    list(REMOVE_DUPLICATES FV_USER_DIRS)
    target_sources(Code_visualiser PRIVATE ${FV_USER_SOURCES})
    target_include_directories(Code_visualiser PRIVATE ${FV_USER_DIRS})
    target_compile_definitions(Code_visualiser PRIVATE FV_USER_PROGRAM)
    source_group("Instrumented program" FILES ${FV_USER_SOURCES})
endif()

target_link_libraries(Code_visualiser OgreBites Threads::Threads)

source_group("Source Files" FILES ${CV_SOURCES})
source_group("Header Files" FILES ${CV_HEADERS})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY VS_STARTUP_PROJECT Code_visualiser)
set_target_properties(Code_visualiser PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    set(CV_DLL_SOURCE_DIRS "")
    foreach(ogre_target OgreMain OgreBites OgreRTShaderSystem OgreOverlay)
        if(TARGET ${ogre_target})
            get_target_property(ogre_dll ${ogre_target} IMPORTED_LOCATION_RELEASE)
            if(NOT ogre_dll)
                get_target_property(ogre_dll ${ogre_target} IMPORTED_LOCATION)
            endif()
            if(ogre_dll AND ogre_dll MATCHES "[.]dll$")
                get_filename_component(ogre_dll_dir "${ogre_dll}" DIRECTORY)
                list(APPEND CV_DLL_SOURCE_DIRS "${ogre_dll_dir}")
            endif()
        endif()
    endforeach()
    list(REMOVE_DUPLICATES CV_DLL_SOURCE_DIRS)

    set(CV_DLLS "")
    foreach(dll_dir ${CV_DLL_SOURCE_DIRS})
        file(GLOB found_dlls "${dll_dir}/*.dll")
        list(APPEND CV_DLLS ${found_dlls})
    endforeach()

    if(CV_DLLS)
        message(STATUS "Ogre DLLs will be copied from: ${CV_DLL_SOURCE_DIRS}")
        add_custom_command(TARGET Code_visualiser POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CV_DLLS} $<TARGET_FILE_DIR:Code_visualiser>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CV_DLLS} ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Copying Ogre DLLs from the Ogre SDK")
    else()
        message(STATUS "Ogre DLL folder not found from CMake; using the DLLs in ${CMAKE_CURRENT_SOURCE_DIR}")
        file(GLOB CV_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/*.dll")
        if(CV_DLLS)
            add_custom_command(TARGET Code_visualiser POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CV_DLLS} $<TARGET_FILE_DIR:Code_visualiser>
                COMMENT "Copying Ogre DLLs next to the executable")
        else()
            message(WARNING "No Ogre DLLs found. Copy the DLLs from your Ogre SDK (bin/Release) into ${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
    endif()
endif()

option(CV_BUILD_TESTS "Build the tests" ON)
if(CV_BUILD_TESTS)
    enable_testing()
    add_subdirectory(${CV_ROOT}/tests ${CMAKE_BINARY_DIR}/tests)
endif()
