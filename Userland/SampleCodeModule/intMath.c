#include <intMath.h>

int64_t sin(int angle) {
    double x = (angle * PI) / 180.0;
    
    double x2 = x * x;
    double x3 = x2 * x;
    double x5 = x3 * x2;
    double x7 = x5 * x2;
    double x9 = x7 * x2;
    double x11 = x9 * x2;
    
    double result = x - x3/6 + x5/120 - x7/5040 + x9/362880 - x11/39916800;
                   
    return (int64_t)(result * MATH_SCALE);
}

int64_t cos(int angle) {
    double x = (angle * PI) / 180.0;
    
    double x2 = x * x;
    double x4 = x2 * x2;
    double x6 = x4 * x2;
    double x8 = x6 * x2;
    double x10 = x8 * x2;
    
    double result = 1 - x2/2 + x4/24 - x6/720 + x8/40320 - x10/3628800;
                   
    return (int64_t)(result * MATH_SCALE);
}

point2D rotatePoint(point2D original, int angle) {
    //x' = x*cos(t) - y*sin(t)
    //y' = x*sin(t) + y*cos(t)
    double outX = (original.x * cos(angle) - original.y * sin(angle))/(double)MATH_SCALE;
    double outY = (original.x * sin(angle) + original.y * cos(angle))/(double)MATH_SCALE;
    return point(outX,outY);
}

point2D setMagnitude(point2D p, int targetMagnitude) {
    point2D out = pointScale(p, targetMagnitude);
    out = pointScale(out, 1/(double)magnitude(p));
    return out;
}

uint64_t abs(int64_t n) {
    int mask = n >> 63;
    return (n ^ mask) - mask;
}

uint64_t magnitude(point2D point) {
    float sqSum = squareMagnitude(point);

    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = sqSum * 0.5F;
    y = sqSum;
    i = *(long*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    y = y * (threehalfs - (x2 * y * y));
    y = y * (threehalfs - (x2 * y * y));

    return abs(1/y);
}