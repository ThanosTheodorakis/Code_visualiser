// DemoProgram.h - The built-in demo program that runs when no other program is given.

#pragma once

#include <iostream>

#include "Markers.h"
#include "Instrument.h"

// The built-in demo program, written directly with markers.
namespace demo {
// Returns x squared.
inline int square(int x)
{
    FV_FUNCTION(x);
    return FV_RETURN(x * x);
}

// Weight of i by its remainder modulo 3 (a switch example).
inline int weight(int i)
{
    FV_FUNCTION(i);
    int w = 0;
    FV_SWITCH(i % 3)
    {
        case 0:
            w = 3;
            break;
        case 1:
            w = 1;
            break;
        default:
            w = 2;
    }
    return FV_RETURN(w);
}

// Asks how many squares to sum and reads the answer (1 to 20, default 3).
inline int readCount(std::istream& in)
{
    FV_FUNCTION();
    int n = 3;
    FV_OUTPUT(std::cout << "how many squares (1-20)? " << std::flush);
    FV_INPUT(in >> n);
    return FV_RETURN((n >= 1 && n <= 20) ? n : 3);
}

// Sums the first n squares with a for loop, a do-while loop and an if.
inline int sumOfSquares(int n)
{
    FV_FUNCTION(n);
    int total = 0;
    int evenSteps = 0;
    int weighted = 0;
    for (int i = 1; i <= n; ++i) {
        FV_STMT(total += square(i));
        FV_STMT(weighted += weight(i));
        if (i % 2 == 0) {
            FV_STMT(++evenSteps);
        }
    }
    int rounds = 0;
    int left = 2;
    FV_DO_WHILE(--left > 0)
    {
        FV_STMT(++rounds);
    }
    FV_OUTPUT(std::cout << "  even steps: " << evenSteps << std::endl);
    return FV_RETURN(total);
}

// Runs the whole demo and returns the total.
inline int runDemo(std::istream& in = std::cin)
{
    FV_FUNCTION();
    const int count = readCount(in);
    const int total = sumOfSquares(count);
    const bool bigAndMany = FV_COND(total > 10) && FV_COND(count > 1);
    const char* size = FV_COND(total > 100) ? "huge" : "normal";
    FV_STMT(size = bigAndMany ? size : "small");
    if (total > 10) {
        FV_OUTPUT(std::cout << "big: " << total << std::endl);
    } else {
        FV_OUTPUT(std::cout << "small: " << total << std::endl);
    }
    return FV_RETURN(total);
}
}

#include "InstrumentEnd.h"
