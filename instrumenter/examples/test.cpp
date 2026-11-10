// test.cpp - Example program for the instrumenter: input, a function call, if/else, for and while.

#include <iostream>

int add(int a, int b)
{
    return a + b;
}

int main()
{
    int a = 0;
    int b = 0;

    std::cout << "first number: ";
    std::cin >> a;
    std::cout << "second number: ";
    std::cin >> b;

    int sum = add(a, b);
    std::cout << "sum = " << sum << std::endl;

    if (sum % 2 == 0) {
        std::cout << "the sum is even" << std::endl;
    } else {
        std::cout << "the sum is odd" << std::endl;
    }

    if (sum > 10) {
        std::cout << "the sum is big" << std::endl;
    } else if (sum > 0) {
        std::cout << "the sum is small" << std::endl;
    } else {
        std::cout << "the sum is not positive" << std::endl;
    }

    int total = 0;
    for (int i = 1; i <= 4; ++i) {
        total += a * i;
        if (i == 2) {
            std::cout << "halfway, total so far = " << total << std::endl;
        }
    }
    std::cout << "a * (1+2+3+4) = " << total << std::endl;

    int countdown = 3;
    while (countdown > 0) {
        --countdown;
    }

    std::cout << "done" << std::endl;
    return 0;
}
