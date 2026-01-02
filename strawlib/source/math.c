#include <math.h>

double fabs(double x) { return x < 0 ? -x : x; }

double fmod(double x, double y) {
    return x - (long)(x / y) * y;
}

double floor(double x) {
    long i = (long)x;
    return (x < 0 && x != i) ? i - 1 : i;
}

double ceil(double x) {
    long i = (long)x;
    return (x > 0 && x != i) ? i + 1 : i;
}

double trunc(double x) {
    return (long)x;
}

double round(double x) {
    return x >= 0 ? floor(x + 0.5) : ceil(x - 0.5);
}

double sqrt(double x) {
    if (x < 0) return NAN;
    double g = x;
    for (int i = 0; i < 25; i++)
        g = 0.5 * (g + x / g);
    return g;
}

double cbrt(double x) {
    double g = x;
    for (int i = 0; i < 25; i++)
        g = (2.0 * g + x / (g * g)) / 3.0;
    return g;
}

double exp(double x) {
    double sum = 1, term = 1;
    for (int i = 1; i < 30; i++) {
        term *= x / i;
        sum += term;
    }
    return sum;
}

double exp2(double x) {
    return exp(x * 0.6931471805599453);
}

double log(double x) {
    if (x <= 0) return NAN;
    double y = x - 1;
    for (int i = 0; i < 25; i++)
        y -= (exp(y) - x) / exp(y);
    return y;
}

double log10(double x) {
    return log(x) / 2.302585092994046;
}

double log2(double x) {
    return log(x) / 0.6931471805599453;
}

double pow(double a, double b) {
    return exp(b * log(a));
}

double sin(double x) {
    double t = x, s = x;
    for (int i = 1; i < 10; i++) {
        t *= -x * x / ((2*i)*(2*i+1));
        s += t;
    }
    return s;
}

double cos(double x) {
    double t = 1, s = 1;
    for (int i = 1; i < 10; i++) {
        t *= -x * x / ((2*i-1)*(2*i));
        s += t;
    }
    return s;
}

double tan(double x) {
    return sin(x) / cos(x);
}

double atan(double x) {
    double s = x;
    double t = x;
    for (int i = 1; i < 15; i++) {
        t *= -x * x;
        s += t / (2*i + 1);
    }
    return s;
}

double atan2(double y, double x) {
    if (x > 0) return atan(y / x);
    if (x < 0 && y >= 0) return atan(y / x) + PI;
    if (x < 0 && y < 0) return atan(y / x) - PI;
    if (x == 0 && y > 0) return PI / 2;
    if (x == 0 && y < 0) return -PI / 2;
    return 0;
}

double asin(double x) {
    return atan(x / sqrt(1 - x*x));
}

double acos(double x) {
    return PI/2 - asin(x);
}

double sinh(double x) {
    return (exp(x) - exp(-x)) / 2;
}

double cosh(double x) {
    return (exp(x) + exp(-x)) / 2;
}

double tanh(double x) {
    return sinh(x) / cosh(x);
}

double hypot(double x, double y) {
    return sqrt(x*x + y*y);
}

double copysign(double x, double y) {
    return y < 0 ? -fabs(x) : fabs(x);
}
