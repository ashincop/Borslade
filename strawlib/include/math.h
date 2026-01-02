#ifndef MATH_H
#define MATH_H

#define PI   3.14159265358979323846
#define E    2.71828182845904523536
#define NAN  (0.0/0.0)
#define INFINITY (1.0/0.0)

/* basic */
double fabs(double);
double fmod(double, double);

/* power / exp */
double sqrt(double);
double cbrt(double);
double pow(double, double);
double exp(double);
double exp2(double);
double log(double);
double log10(double);
double log2(double);

/* trig */
double sin(double);
double cos(double);
double tan(double);
double asin(double);
double acos(double);
double atan(double);
double atan2(double, double);

/* hyperbolic */
double sinh(double);
double cosh(double);
double tanh(double);

/* rounding */
double floor(double);
double ceil(double);
double trunc(double);
double round(double);

/* misc */
double hypot(double, double);
double copysign(double, double);

#endif
