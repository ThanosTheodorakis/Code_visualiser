// LiveSession.h - Runs the program on its own thread and reports how it ended.

#pragma once

#include <atomic>
#include <exception>
#include <functional>
#include <iostream>
#include <thread>
#include <utility>

#include "InterruptibleInput.h"
#include "Markers.h"

// Runs the program on its own thread and reports how it ended.
class LiveSession
{
public:
    enum Outcome
    {
        NotStarted,
        Running,
        Finished,
        Stopped,
        Failed
    };

    explicit LiveSession(std::function<void()> program) : program_(std::move(program)) {}

    ~LiveSession() { stopAndJoin(); }

    LiveSession(const LiveSession&) = delete;
    LiveSession& operator=(const LiveSession&) = delete;

    // Redirects std::cin and starts the program thread. From then on two threads work: this one (the window,
    //     which only reads the event log) and the program thread (which writes it).
    void start()
    {
        outcome_.store(Running);
        input_.install();
        thread_ = std::thread([this] { run(); });
    }

    // Closing the window must never leave the program thread blocked, so it is woken wherever it waits:
    // requestStop() makes the next marker throw StopRequested and tells the handlers to release the
    // renderer's gate (the program may be waiting for capacity there); interrupt() ends a pending read of
    // std::cin. Only then is the thread joined and the original std::cin restored.
    void stopAndJoin()
    {
        if (thread_.joinable()) {
            Trackers::requestStop();
            input_.interrupt();
            thread_.join();
            input_.restore();
        }
    }

    // How the program ended, or Running.
    Outcome outcome() const { return static_cast<Outcome>(outcome_.load()); }

private:
    // Thread body: runs the program and records how it ended.
    void run()
    {
        try {
            {
                ProgramScope program;
                program_();
            }
            outcome_.store(Finished);
        } catch (const StopRequested&) {
            outcome_.store(Stopped);
        } catch (const std::exception& e) {
            std::cerr << "[program] exception: " << e.what() << std::endl;
            outcome_.store(Failed);
        } catch (...) {
            std::cerr << "[program] unknown exception" << std::endl;
            outcome_.store(Failed);
        }
    }

    std::function<void()> program_;
    InterruptibleInput input_;
    std::thread thread_;
    std::atomic<int> outcome_{NotStarted};
};
