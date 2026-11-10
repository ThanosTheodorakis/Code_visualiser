// Backlog.h - Thread-safe gate between the running program and the renderer; holds pause, speed and queue limit.

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

// Lets the program run only as far ahead of the animation as allowed; also holds pause and speed.
// Handoff between the two threads: the program thread calls waitForCapacity() before it adds an event and
// blocks while the gate is paused or too many events and animations are outstanding. The render thread
// changes the counters (setQueued, publishAnimations) and the pause state; each change calls wake(), so
// the blocked program thread checks again. The counters are atomics; the mutex and the condition
// variable are only used to sleep and wake.
class BacklogGate
{
public:
    // Counts a new event waiting to be played.
    void noteEnqueued() { queuedEvents_.fetch_add(1); }

    // Blocks the program while paused or too many events are outstanding.
    void waitForCapacity()
    {
        if (!mustWait()) {
            return;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        changed_.wait(lock, [this] { return !mustWait(); });
    }

    // Lets the program run freely (used when closing).
    void release()
    {
        released_.store(true);
        wake();
    }

    // True after release.
    bool released() const { return released_.load(); }

    // Sets the number of events not yet read by the renderer.
    void setQueued(int queuedEvents)
    {
        queuedEvents_.store(queuedEvents);
        wake();
    }

    // Sets the number of animations still waiting or playing.
    void publishAnimations(int animationsOutstanding)
    {
        animsOutstanding_.store(animationsOutstanding);
        wake();
    }

    // Events and animations the renderer has not finished.
    int outstanding() const { return queuedEvents_.load() + animsOutstanding_.load(); }

    // Sets how many may be outstanding before the program waits.
    void setLimit(int limit)
    {
        limit_.store(limit < 1 ? 1 : limit);
        wake();
    }

    // The current limit.
    int limit() const { return limit_.load(); }

    // Sets the playback speed within its minimum and maximum.
    void setSpeed(float s)
    {
        if (s < kMinSpeed) {
            s = kMinSpeed;
        }
        if (s > kMaxSpeed) {
            s = kMaxSpeed;
        }
        speed_.store(s);
    }

    // The playback speed.
    float speed() const { return speed_.load(); }

    // Doubles the speed.
    void speedUp() { setSpeed(speed() * 2.0f); }

    // Halves the speed.
    void slowDown() { setSpeed(speed() * 0.5f); }

    // Pauses or resumes.
    void togglePause()
    {
        paused_.store(!paused_.load());
        wake();
    }

    // True while paused.
    bool paused() const { return paused_.load(); }

    // Animation time factor: 0 while paused, else the speed.
    float timeScale() const { return paused_.load() ? 0.0f : speed_.load(); }

    static constexpr float kMinSpeed = 0.125f;
    static constexpr float kMaxSpeed = 64.0f;

private:
    // True while the program has to wait.
    bool mustWait() const { return !released_.load() && (paused_.load() || outstanding() >= limit_.load()); }

    // Wakes the waiting program thread after a state change. Taking the mutex first matters: the waiter
    // either has not started to wait yet (it will check the new state) or already waits (it gets the
    // notification), so no wake-up is lost.
    void wake()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
        }
        changed_.notify_all();
    }

    std::mutex mutex_;
    std::condition_variable changed_;
    std::atomic<int> queuedEvents_{0};
    std::atomic<int> animsOutstanding_{0};
    std::atomic<int> limit_{8};
    std::atomic<bool> released_{false};
    std::atomic<bool> paused_{false};
    std::atomic<float> speed_{1.0f};
};
