#ifndef POINT_H
#define POINT_H
#include <stdint.h>

#define ORIGIN (point2D){0,0}

typedef struct {
    int64_t x;
    int64_t y;
} point2D;

point2D point(int64_t x, int64_t y);
point2D pointAdd(point2D p1, point2D p2);
point2D pointSub(point2D p1, point2D p2);
point2D pointScale(point2D p, double scalar);
int64_t squareDistance(point2D p1, point2D p2);
int64_t squareMagnitude(point2D p1);
int pointEquals(point2D p1, point2D p2);
// Retorna distinto de 0 si |p1-p2|^2 < sqrThreshold
int pointClose(point2D p1, point2D p2, int sqrThreshold);

#endif