long double pasgfact(long double a, long double b, long double c, long double d)
{
    long double res = 1.0;
    while (a > 1.0)
    {
        res = res * a;
        a = a - 1.0;
        if (b > 1.0)
        {
            res = res / b;
            b = b - 1.0;
        }

        if (c > 1.0)
        {
            res = res / c;
            c = c - 1.0;
        }

        if (d > 1.0)
        {
            res = res / d;
            d = d - 1.0;
        }
    }
    return res;
}