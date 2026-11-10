// InterruptibleInput.h - Replaces std::cin's buffer so a program waiting for input can be stopped.

#pragma once

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <streambuf>
#include <string>
#include <thread>

// A stream buffer for std::cin that a stop request can wake while the program waits for input.
// A blocking read of std::cin cannot be cancelled, so a reader thread does the real read, one line at a
// time and only when the program asks (wanted). The program thread waits on a condition variable for the
// line (pending) or for interrupt(). The reader may stay blocked in a read after the window has closed, so
// it is detached and holds the shared state through a shared_ptr, which keeps the state alive for it.
class InterruptibleInput : public std::streambuf
{
public:
    InterruptibleInput() : shared_(sharedState()) {}

    ~InterruptibleInput() override { restore(); }

    InterruptibleInput(const InterruptibleInput&) = delete;
    InterruptibleInput& operator=(const InterruptibleInput&) = delete;

    // Replaces the stream buffer of the stream (std::cin by default).
    void install(std::istream& stream = std::cin)
    {
        if (stream_) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(shared_->mutex);
            shared_->interrupted = false;
        }
        stream_ = &stream;
        original_ = stream.rdbuf(this);
        if (!shared_->source) {
            shared_->source = original_;
        }
    }

    // Wakes a waiting read so it ends as if the input closed.
    void interrupt()
    {
        {
            std::lock_guard<std::mutex> lock(shared_->mutex);
            shared_->interrupted = true;
        }
        shared_->changed.notify_all();
    }

    // Puts the original stream buffer back.
    void restore()
    {
        if (stream_) {
            stream_->rdbuf(original_);
            stream_->clear();
            stream_ = nullptr;
            original_ = nullptr;
        }
    }

protected:
    // Supplies more characters, waiting for a line from the reader thread.
    int_type underflow() override
    {
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }

        std::unique_lock<std::mutex> lock(shared_->mutex);
        for (;;) {
            if (shared_->interrupted) {
                return traits_type::eof();
            }
            if (!shared_->pending.empty()) {
                chunk_ = std::move(shared_->pending);
                shared_->pending.clear();
                char* first = &chunk_[0];
                setg(first, first, first + chunk_.size());
                return traits_type::to_int_type(*first);
            }
            if (shared_->endOfInput) {
                return traits_type::eof();
            }
            if (!shared_->wanted) {
                shared_->wanted = true;
                if (!shared_->readerStarted) {
                    shared_->readerStarted = true;
                    std::thread(readLines, shared_).detach();
                }
                shared_->changed.notify_all();
            }
            shared_->changed.wait(lock);
        }
    }

private:
    // State shared with the reader thread, which can outlive this object.
    struct Shared
    {
        std::mutex mutex;
        std::condition_variable changed;
        std::string pending;
        std::streambuf* source = nullptr;
        bool wanted = false;
        bool readerStarted = false;
        bool endOfInput = false;
        bool interrupted = false;
    };

    // The one shared state of the process.
    static std::shared_ptr<Shared> sharedState()
    {
        static std::shared_ptr<Shared> state = std::make_shared<Shared>();
        return state;
    }

    // Reader thread: reads one line of real input each time one is wanted.
    static void readLines(std::shared_ptr<Shared> shared)
    {
        for (;;) {
            {
                std::unique_lock<std::mutex> lock(shared->mutex);
                shared->changed.wait(lock, [&] { return shared->wanted; });
            }

            std::string line;
            bool ended = false;
            for (;;) {
                const int c = shared->source->sbumpc();
                if (c == traits_type::eof()) {
                    ended = true;
                    break;
                }
                line.push_back(static_cast<char>(c));
                if (c == '\n') {
                    break;
                }
            }

            {
                std::lock_guard<std::mutex> lock(shared->mutex);
                shared->pending += line;
                shared->endOfInput = shared->endOfInput || ended;
                shared->wanted = false;
            }
            shared->changed.notify_all();
        }
    }

    std::shared_ptr<Shared> shared_;
    std::istream* stream_ = nullptr;
    std::streambuf* original_ = nullptr;
    std::string chunk_;
};
