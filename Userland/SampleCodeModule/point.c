/*
 * point.c — Constructor liviano para el TAD `point2D` (x, y).
 */
#include <point.h>
#include <stdint.h>

point2D point(int64_t x, int64_t y) {
    point2D p = {x,y};
    return p;
}

point2D pointAdd(point2D p1, point2D p2) {
    return point(p1.x + p2.x, p1.y + p2.y);
}

point2D pointSub(point2D p1, point2D p2) {
    return point(p1.x - p2.x, p1.y - p2.y);
}

point2D pointScale(point2D p, double scalar) {
    return point(p.x*scalar, p.y*scalar);
}

int64_t squareDistance(point2D p1, point2D p2) {
    int64_t dx = p1.x - p2.x;
    int64_t dy = p1.y - p2.y;
    return dx * dx + dy * dy;
}

int64_t squareMagnitude(point2D p) {
    return p.x*p.x + p.y*p.y;
}

int pointEquals(point2D p1, point2D p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

int pointClose(point2D p1, point2D p2, int sqrThreshold) {
    return squareDistance(p1,p2) < sqrThreshold;
}