// backlog_test.cpp - Tests for the backlog gate: speed, pause and waiting between program and renderer.

#include <atomic>
#include <chrono>
#include <thread>

#include "Backlog.h"
#include "test_util.h"

static void speedTests()
{
    BacklogGate gate;
    CHECK_EQ(gate.speed(), 1.0f);

    gate.speedUp();
    CHECK_EQ(gate.speed(), 2.0f);
    gate.slowDown();
    gate.slowDown();
    CHECK_EQ(gate.speed(), 0.5f);

    for (int i = 0; i < 20; ++i) {
        gate.speedUp();
    }
    CHECK_EQ(gate.speed(), BacklogGate::kMaxSpeed);

    for (int i = 0; i < 30; ++i) {
        gate.slowDown();
    }
    CHECK_EQ(gate.speed(), BacklogGate::kMinSpeed);

    gate.setSpeed(1000.0f);
    CHECK_EQ(gate.speed(), BacklogGate::kMaxSpeed);
    gate.setSpeed(-3.0f);
    CHECK_EQ(gate.speed(), BacklogGate::kMinSpeed);
}

static void pauseTests()
{
    BacklogGate gate;
    gate.setSpeed(4.0f);
    CHECK(!gate.paused());
    CHECK_EQ(gate.timeScale(), 4.0f);

    gate.togglePause();
    CHECK(gate.paused());
    CHECK_EQ(gate.timeScale(), 0.0f);
    CHECK_EQ(gate.speed(), 4.0f);

    gate.togglePause();
    CHECK(!gate.paused());
    CHECK_EQ(gate.timeScale(), 4.0f);
}

static void limitAndCountTests()
{
    BacklogGate gate;
    CHECK_EQ(gate.limit(), 8);

    gate.setLimit(0);
    CHECK_EQ(gate.limit(), 1);
    gate.setLimit(-5);
    CHECK_EQ(gate.limit(), 1);
    gate.setLimit(20);
    CHECK_EQ(gate.limit(), 20);

    CHECK_EQ(gate.outstanding(), 0);
    gate.noteEnqueued();
    gate.noteEnqueued();
    CHECK_EQ(gate.outstanding(), 2);
    gate.publishAnimations(3);
    CHECK_EQ(gate.outstanding(), 5);
    gate.setQueued(0);
    CHECK_EQ(gate.outstanding(), 3);
}

static void waitingTests()
{
    using namespace std::chrono;

    {
        BacklogGate gate;
        gate.setLimit(4);
        const auto start = steady_clock::now();
        gate.waitForCapacity();
        CHECK(steady_clock::now() - start < milliseconds(200));
    }

    {
        BacklogGate gate;
        gate.setLimit(2);
        gate.setQueued(2);
        std::atomic<bool> passed{false};
        std::thread program([&] {
            gate.waitForCapacity();
            passed = true;
        });

        std::this_thread::sleep_for(milliseconds(50));
        CHECK(!passed.load());

        gate.setQueued(1);
        program.join();
        CHECK(passed.load());
    }

    {
        BacklogGate gate;
        gate.togglePause();
        std::atomic<bool> passed{false};
        std::thread program([&] {
            gate.waitForCapacity();
            passed = true;
        });

        std::this_thread::sleep_for(milliseconds(50));
        CHECK(!passed.load());

        gate.togglePause();
        program.join();
        CHECK(passed.load());
    }

    {
        BacklogGate gate;
        gate.setLimit(1);
        gate.setQueued(10);
        gate.togglePause();
        std::atomic<bool> passed{false};
        std::thread program([&] {
            gate.waitForCapacity();
            passed = true;
        });

        std::this_thread::sleep_for(milliseconds(50));
        CHECK(!passed.load());

        gate.release();
        CHECK(gate.released());
        program.join();
        CHECK(passed.load());
    }
}

int main()
{
    speedTests();
    pauseTests();
    limitAndCountTests();
    waitingTests();
    return finish("backlog_test");
}
