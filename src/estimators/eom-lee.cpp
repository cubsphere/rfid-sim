#include <cmath>
#include <algorithm>

using namespace std;

long double computeB(long double Y, int window_size, int successes, int collisions)
{
    return window_size / (Y * collisions + successes);
}

long double computeY(long double B)
{
    if ((B == -INFINITY) | (B == INFINITY))
        return -2;
    
    long double dividend = 1 - exp(-(1 / B));
    long double divisor = B * (1 - (1 + 1 / B) * exp(-(1 / B)));
    return dividend / divisor;
}

static const long double epsilon = 0.00390625;

int eom_lee(int empties, int successes, int collisions)
{
    int window_size = empties + successes + collisions;
    long double prevY = -2;

    long double B = computeB(prevY, window_size, successes, collisions);
    long double Y = computeY(B);
    while (epsilon < abs(Y - prevY))
    {
        prevY = Y;
        B = computeB(prevY, window_size, successes, collisions);
        Y = computeY(B);
    }

    return max(1, (int) ceil(Y) * collisions);
}