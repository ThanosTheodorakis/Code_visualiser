// sample.cpp - Example program for the instrumenter: switch, do/while, && and ?: conditions.

#include <iostream>
#include <string>
#include <vector>

enum class Mode
{
    Fast,
    Slow
};

struct Counter
{
    int hits = 0;

    Counter() {}

    void add(int n) { hits += n; }

    int total() const { return hits; }
};

static int square(int x)
{
    return x * x;
}

static std::string grade(int score)
{
    if (score >= 90) {
        return "A";
    } else if (score >= 75 && score < 90) {
        return "B";
    } else if (score >= 50) {
        return "C";
    }
    return "F";
}

static int weight(int i)
{
    int w = 0;
    switch (i % 3) {
        case 0:
            w = 3;
            break;
        case 1:
            w = 1;
            break;
        default:
            w = 2;
    }
    return w;
}

template <class T> T biggest(T a, T b)
{
    return a > b ? a : b;
}

constexpr int twice(int v)
{
    return v > 0 ? v * 2 : 0;
}

int main()
{
    int n = 4;
    std::cout << "how many? ";
    std::cin >> n;

    Counter counter;
    std::vector<int> values;
    int sum = 0;
    for (int i = 1; i <= n; ++i) {
        sum += square(i) * weight(i);
        values.push_back(i);
        counter.add(1);
    }

    int left = 3;
    do {
        --left;
        sum += left;
    } while (left > 0);

    if (int extra = sum % 7; extra > 3) {
        std::cout << "extra " << extra << "\n";
    } else if (sum > 100 || n > 10) {
        std::cout << "big\n";
    }

    int idx = 0;
    while (idx < 3 && idx != 5) {
        ++idx;
    }

    for (int v : values) {
        sum += v;
    }

    std::string g = grade(sum % 100);
    std::cout << "grade " << g << " " << biggest(sum, 10) << " " << twice(3) << std::endl;
    Mode mode = sum > 50 ? Mode::Fast : Mode::Slow;
    (void)mode;
    return counter.total() > 0 ? 0 : 1;
}
