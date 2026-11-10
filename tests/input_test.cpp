// input_test.cpp - Tests that a read waiting for input can be interrupted and that normal input still works.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <istream>
#include <mutex>
#include <streambuf>
#include <string>
#include <thread>

#include "InterruptibleInput.h"
#include "test_util.h"

class SlowSource : public std::streambuf
{
public:
    void provide(const std::string& text)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            data_ += text;
        }
        changed_.notify_all();
    }

protected:
    int_type underflow() override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        changed_.wait(lock, [this] { return !data_.empty(); });
        buffer_ = std::move(data_);
        data_.clear();
        setg(&buffer_[0], &buffer_[0], &buffer_[0] + buffer_.size());
        return traits_type::to_int_type(buffer_[0]);
    }

private:
    std::mutex mutex_;
    std::condition_variable changed_;
    std::string data_;
    std::string buffer_;
};

int main()
{
    using namespace std::chrono;

    // Deliberately never deleted: the detached reader thread may still be blocked in this source.
    SlowSource& source = *new SlowSource;
    std::istream stream(&source);
    InterruptibleInput input;
    input.install(stream);

    source.provide("42 hello\n");
    int number = 0;
    std::string word;
    stream >> number >> word;
    CHECK_EQ(number, 42);
    CHECK_EQ(word, "hello");
    CHECK(stream.good());

    std::string line;
    std::getline(stream, line);
    CHECK_EQ(line, "");

    std::atomic<bool> finished{false};
    std::atomic<bool> failed{false};
    std::thread program([&] {
        int value = 0;
        stream >> value;
        failed = stream.fail();
        finished = true;
    });

    std::this_thread::sleep_for(milliseconds(100));
    CHECK(!finished.load());

    const auto start = steady_clock::now();
    input.interrupt();
    program.join();
    CHECK(finished.load());
    CHECK(failed.load());
    CHECK(steady_clock::now() - start < seconds(2));

    input.restore();
    CHECK(stream.rdbuf() == &source);
    CHECK(stream.good());

    return finish("input_test");
}
