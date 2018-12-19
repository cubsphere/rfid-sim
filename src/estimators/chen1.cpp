#include <cmath>
#include "pasgfact.hpp"

static const long double epsilon = 0.03125;

int chen1(int empties, int successes, int collisions)
{
    long double L = empties + successes + collisions;
    long double n = successes + 2.0 * collisions;
    long double next = 0.0;
    long double prev = -1.0;

    while (prev < next - epsilon)
    {
        long double pe = powl((1.0 - (1.0 / L)), n);
        long double ps = (n / L) * powl((1.0 - (1.0 / L)), n - 1.0);
        long double pc = 1.0 - pe - ps;
        prev = next;
        next = pasgfact(L, empties, successes, collisions) * powl(pe, empties) * powl(ps, successes) * powl(pc, collisions);
        n += 1.0;
    }
    return (n - 2.0);
}