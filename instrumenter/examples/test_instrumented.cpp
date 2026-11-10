// Instrumented by fv_instrument.py from test.cpp
// test.cpp - Example program for the instrumenter: input, a function call, if/else, for and while.

#include <iostream>
#include "Instrument.h"

int add(int a, int b)
{
    FV_FUNCTION(a, b);
    return FV_RETURN(a + b);
}

int fv_user_main()
{
    FUNCTION_SCOPE("main");
    FV_DECL_AS("int a = 0"); int a = 0;
    FV_DECL_AS("int b = 0"); int b = 0;

    FV_OUTPUT(std::cout << "first number: ");
    FV_INPUT(std::cin >> a);
    FV_OUTPUT(std::cout << "second number: ");
    FV_INPUT(std::cin >> b);

    FV_DECL_AS("int sum = add(a, b)"); int sum = add(a, b);
    FV_OUTPUT(std::cout << "sum = " << sum << std::endl);

    if (sum % 2 == 0) {
        FV_OUTPUT(std::cout << "the sum is even" << std::endl);
    } else {
        FV_OUTPUT(std::cout << "the sum is odd" << std::endl);
    }

    if (sum > 10) {
        FV_OUTPUT(std::cout << "the sum is big" << std::endl);
    } else if (sum > 0) {
        FV_OUTPUT(std::cout << "the sum is small" << std::endl);
    } else {
        FV_OUTPUT(std::cout << "the sum is not positive" << std::endl);
    }

    FV_DECL_AS("int total = 0"); int total = 0;
    for (int i = 1; i <= 4; ++i) {
        FV_STMT_AS("total += a * i", total += a * i);
        if (i == 2) {
            FV_OUTPUT(std::cout << "halfway, total so far = " << total << std::endl);
        }
    }
    FV_OUTPUT(std::cout << "a * (1+2+3+4) = " << total << std::endl);

    FV_DECL_AS("int countdown = 3"); int countdown = 3;
    while (countdown > 0) {
        FV_STMT_AS("--countdown", --countdown);
    }

    FV_OUTPUT(std::cout << "done" << std::endl);
    return FV_RETURN(0);
}

#include "InstrumentEnd.h"

int fvUserProgram()
{
    return fv_user_main();
}
