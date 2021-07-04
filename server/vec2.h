#ifndef VEC2_H
#define VEC2_H

typedef struct Vec2 {
        float x;
        float y;
} Vec2;

Vec2 Vec2Add(Vec2 a, Vec2 b);
Vec2 Vec2Sub(Vec2 a, Vec2 b);
Vec2 Vec2Mul(Vec2 a, float b);
Vec2 Vec2Div(Vec2 a, float b);
float Vec2Length(Vec2 v);
Vec2 Vec2Normalize(Vec2 v);

#endif