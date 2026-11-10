# CMakeLists.txt - Builds the tests (most need no Ogre) and registers them with ctest.

cmake_minimum_required(VERSION 3.10)
project(CodeVisualiserTests CXX)
enable_testing()

find_package(Threads REQUIRED)
set(CV_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

function(cv_add_test name)
    add_executable(${name} ${name}.cpp ${ARGN})
    target_include_directories(${name} PRIVATE "${CV_ROOT}/include" "${CMAKE_CURRENT_LIST_DIR}")
    target_link_libraries(${name} PRIVATE Threads::Threads)
    set_target_properties(${name} PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

cv_add_test(demo_test    "${CV_ROOT}/src/Markers.cpp")
cv_add_test(markers_test "${CV_ROOT}/src/Markers.cpp")
cv_add_test(backlog_test)
cv_add_test(input_test)
cv_add_test(shapes_test)
cv_add_test(runfile_test)

cv_add_test(eventlog_test "${CV_ROOT}/src/RenderEvents.cpp" "${CV_ROOT}/src/EventLog.cpp"
            "${CV_ROOT}/src/CallStackTracker.cpp" "${CV_ROOT}/src/RunStore.cpp")
set_target_properties(eventlog_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
cv_add_test(speed_test   "${CV_ROOT}/src/Markers.cpp")

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME instrumenter_test
             COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/instrumenter_test.py")
    set_tests_properties(instrumenter_test PROPERTIES SKIP_RETURN_CODE 77)
endif()

option(CV_TEST_CONFIG "Also build config_test (needs Ogre)" OFF)
if(CV_TEST_CONFIG AND TARGET OgreMain)
    add_executable(config_test config_test.cpp "${CV_ROOT}/src/configParser.cpp")
    target_include_directories(config_test PRIVATE "${CV_ROOT}/include" "${CMAKE_CURRENT_LIST_DIR}")
    target_link_libraries(config_test PRIVATE OgreMain)
    target_compile_definitions(config_test PRIVATE
        CV_SHIPPED_CONFIG="${CV_ROOT}/app/config.cfg")
    set_target_properties(config_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
    add_custom_command(TARGET config_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:OgreMain> $<TARGET_FILE_DIR:config_test>)
    add_test(NAME config_test COMMAND config_test)
endif()
