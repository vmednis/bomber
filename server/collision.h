#ifndef COLLISION_H
#define COLLISION_H

#include <stdbool.h>
#include "../server/vec2.h"

typedef struct Circle {
        Vec2 pos;
        float r;
} Circle;

typedef struct Rect {
        Vec2 pos;
        Vec2 size;
} Rect;

typedef struct Collision {
        bool exists;
        Vec2 correction;
} Collision;

Collision CollisionCircleVsRect(Circle c, Rect r);

#endif