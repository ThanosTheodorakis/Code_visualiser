// speed_test.cpp - Measures how much slower the markers make a recursive fib(27).

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "Markers.h"
#include "test_util.h"

static long long calls = 0;

static int fibPlain(int n)
{
    ++calls;
    if (n < 2) {
        return n;
    }
    return fibPlain(n - 1) + fibPlain(n - 2);
}

static int fibMarked(int n)
{
    FV_FUNCTION(n);
    ++calls;
    if (FV_COND(n < 2)) {
        return FV_RETURN(n);
    }
    return FV_RETURN(fibMarked(n - 1) + fibMarked(n - 2));
}

template <class F> static double timeMs(F f, int& result)
{
    const auto start = std::chrono::steady_clock::now();
    result = f();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main(int argc, char** argv)
{
    const int n = argc > 1 ? std::atoi(argv[1]) : 27;

    Trackers::clearHandlers();

    int a = 0, b = 0;
    calls = 0;
    const double plainMs = timeMs([&] { return fibPlain(n); }, a);
    const long long plainCalls = calls;

    calls = 0;
    const double markedMs = timeMs([&] { return fibMarked(n); }, b);

    std::printf("fib(%d) = %d, %lld calls\n", n, a, plainCalls);
    std::printf("without markers: %9.3f ms  (%.1f ns per call)\n", plainMs, plainMs * 1e6 / plainCalls);
    std::printf("with markers:    %9.3f ms  (%.1f ns per call)\n", markedMs, markedMs * 1e6 / plainCalls);
    std::printf("slowdown:        about %.0f times\n", markedMs / (plainMs > 0 ? plainMs : 0.001));

    CHECK_EQ(a, b);
    CHECK_EQ(calls, plainCalls);
    return finish("speed_test");
}
