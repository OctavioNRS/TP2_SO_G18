#ifndef INTMATH_H
#define INTMATH_H
#include <stdint.h>
#include <point.h>

#define MATH_SCALE 100000
#define PI 3.14159265359

int64_t sin(int angle);
int64_t cos(int angle);
uint64_t abs(int64_t n);
uint64_t magnitude(point2D point);

point2D rotatePoint(point2D original, int angle);
point2D setMagnitude(point2D p, int targetMagnitude);

#endif