// test_util.h - A tiny check macro so the tests need no test library.

#pragma once

#include <cstdio>

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(condition)) {                                                  \
            ++g_failures;                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        }                                                                    \
    } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))

inline int finish(const char* name)
{
    std::printf("%s: %s (%d checks)\n", name, g_failures == 0 ? "OK" : "FAILED", g_checks);
    return g_failures == 0 ? 0 : 1;
}
