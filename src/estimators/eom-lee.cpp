#include <cmath>
#include <algorithm>

using namespace std;

double computeB(double Y, int window_size, int successes, int collisions)
{
    return window_size / (Y * successes + collisions);
}

double computeY(double B)
{
    double dividend = 1 - exp(-(1 / B));
    double divisor = B * (1 - (1 + 1 / B) * exp(-(1 / B)));
    return dividend / divisor;
}

static const double epsilon = 0.00390625;

int eom_lee(int empties, int successes, int collisions)
{
    int window_size = empties + successes + collisions;
    double prevY = -2;

    double B = computeB(prevY, window_size, successes, collisions);
    double Y = computeY(B);
    while (epsilon < abs(Y - prevY))
    {
        prevY = Y;
        B = computeB(prevY, window_size, successes, collisions);
        Y = computeY(B);
    }

    return ceil(Y) * max(1, successes);
}