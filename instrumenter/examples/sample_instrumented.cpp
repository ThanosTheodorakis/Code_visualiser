// Instrumented by fv_instrument.py from sample.cpp
// sample.cpp - Example program for the instrumenter: switch, do/while, && and ?: conditions.

#include <iostream>
#include <string>
#include <vector>
#include "Instrument.h"

enum class Mode
{
    Fast,
    Slow
};

struct Counter
{
    int hits = 0;

    Counter() { FV_FUNCTION();}

    void add(int n) { FV_FUNCTION(n); FV_STMT_AS("hits += n", hits += n); }

    int total() const { FV_FUNCTION(); return FV_RETURN(hits); }
};

static int square(int x)
{
    FV_FUNCTION(x);
    return FV_RETURN(x * x);
}

static std::string grade(int score)
{
    FV_FUNCTION(score);
    if (score >= 90) {
        return FV_RETURN("A");
    } else if (FV_COND(score >= 75) && FV_COND(score < 90)) {
        return FV_RETURN("B");
    } else if (score >= 50) {
        return FV_RETURN("C");
    }
    return FV_RETURN("F");
}

static int weight(int i)
{
    FV_FUNCTION(i);
    FV_DECL_AS("int w = 0"); int w = 0;
    FV_SWITCH (i % 3) {
        case 0:
            FV_STMT_AS("w = 3", w = 3);
            break;
        case 1:
            FV_STMT_AS("w = 1", w = 1);
            break;
        default:
            FV_STMT_AS("w = 2", w = 2);
    }
    return FV_RETURN(w);
}

template <class T> T biggest(T a, T b)
{
    FV_FUNCTION(a, b);
    return FV_COND(a > b) ? a : b;
}


#include "InstrumentEnd.h"
constexpr int twice(int v)
{
    return v > 0 ? v * 2 : 0;
}
#include "Instrument.h"


int fv_user_main()
{
    FUNCTION_SCOPE("main");
    FV_DECL_AS("int n = 4"); int n = 4;
    FV_OUTPUT(std::cout << "how many? ");
    FV_INPUT(std::cin >> n);

    Counter counter;
    std::vector<int> values;
    FV_DECL_AS("int sum = 0"); int sum = 0;
    for (int i = 1; i <= n; ++i) {
        FV_STMT_AS("sum += square(i) * weight(i)", sum += square(i) * weight(i));
        FV_STMT_AS("values.push_back(i)", values.push_back(i));
        FV_STMT_AS("counter.add(1)", counter.add(1));
    }

    FV_DECL_AS("int left = 3"); int left = 3;
    FV_DO_WHILE(left > 0)  {
        FV_STMT_AS("--left", --left);
        FV_STMT_AS("sum += left", sum += left);
    } 

    { int extra = sum % 7; if (extra > 3) {
        FV_OUTPUT(std::cout << "extra " << extra << "\n");
    } else if (FV_COND(sum > 100) || FV_COND(n > 10)) {
        FV_OUTPUT(std::cout << "big\n");
    } }

    FV_DECL_AS("int idx = 0"); int idx = 0;
    while (idx < 3 && idx != 5) {
        FV_STMT_AS("++idx", ++idx);
    }

    for (int v : values) {
        FV_STMT_AS("sum += v", sum += v);
    }

    FV_DECL_AS("std::string g = grade(sum % 100)"); std::string g = grade(sum % 100);
    FV_OUTPUT(std::cout << "grade " << g << " " << biggest(sum, 10) << " " << twice(3) << std::endl);
    FV_DECL_AS("Mode mode = sum > 50 ? Mode::Fast : Mode::Slow"); Mode mode = FV_COND(sum > 50) ? Mode::Fast : Mode::Slow;
    FV_STMT_AS("(void)mode", (void)mode);
    return FV_RETURN(FV_COND(counter.total() > 0) ? 0 : 1);
}

#include "InstrumentEnd.h"

int fvUserProgram()
{
    return fv_user_main();
}
