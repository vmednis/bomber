#include<math.h>
#include "../server/vec2.h"

Vec2 Vec2Add(Vec2 a, Vec2 b) {
        return (Vec2) {a.x + b.x, a.y + b.y};
}

Vec2 Vec2Sub(Vec2 a, Vec2 b) {
        return (Vec2) {a.x - b.x, a.y - b.y};
}

Vec2 Vec2Mul(Vec2 a, float b) {
        return (Vec2) {a.x * b, a.y * b};
}

Vec2 Vec2Div(Vec2 a, float b) {
        return (Vec2) {a.x / b, a.y / b};
}

float Vec2Length(Vec2 v) {
        return sqrt(v.x * v.x + v.y * v.y);
}

Vec2 Vec2Normalize(Vec2 v) {
        return Vec2Div(v, Vec2Length(v));
}