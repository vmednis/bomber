#include<math.h>
#include "../server/vec2.h"
#include "../server/collision.h"

#define COLLISION_NONE (Collision) {false, {0, 0}}
#define PI 3.14159265358979323846f
#define EPSILON 0.0001

static float CollisionCheckAxis(float aa, float ab, float ba, float bb) {
        float tmp;
        float mida, midb;

        if(aa > ab) { tmp = aa; aa = ab; ab = tmp; }
        if(ba > bb) { tmp = ba; ba = bb; bb = tmp; }

        if(aa > bb || ba > ab) {
                return 0;
        }

        mida = aa + (ab - aa) / 2;
        midb = ba + (bb - ba) / 2;

        if(mida > midb) {
                return aa - bb;
        } else {
                return ab - ba;
        }
}

static float CollisionProjectPointOnAxis(Vec2 p, Vec2 op, Vec2 od) {
        float at;

        at = atan(-od.y / od.x);
        if(od.x < 0) {
                if(od.y < 0) {
                        at += PI;
                } else {
                        at -= PI;
                }
        }

        return cos(at) * (p.x - op.x) - sin(at) * (p.y - op.y);
}


Collision CollisionCircleVsRect(Circle c, Rect r) {
        float rl, rr, rt, rb, rc, rf;
        Vec2 dir, disp, mindisp;
        struct CollisionCheck {
                bool check;
                Vec2 close;
                Vec2 far;
        } checks[4];
        float len, minlen;
        int i;

        minlen = INFINITY;
        rl = r.pos.x;
        rt = r.pos.y;
        rr = rl + r.size.x;
        rb = rt + r.size.y;

        //X axis
        len = CollisionCheckAxis(rl, rr, c.pos.x - c.r, c.pos.x + c.r);
        disp = (Vec2) {len, 0};
        len = Vec2Length(disp);
        if(len < EPSILON) {
                return COLLISION_NONE;
        } else {
                minlen = len;
                mindisp = disp;
        }

        //Y axis
        len = CollisionCheckAxis(rt, rb, c.pos.y - c.r, c.pos.y + c.r);
        disp = (Vec2) {0, len};
        len = Vec2Length(disp);
        if(len < EPSILON) {
                return COLLISION_NONE;
        } else if (len < minlen) {
                minlen = len;
                mindisp = disp;
        }

        //Corners
        checks[0] = (struct CollisionCheck) {rl > c.pos.x && rt > c.pos.y, {rl, rt}, {rr, rb}};
        checks[1] = (struct CollisionCheck) {rr < c.pos.x && rt > c.pos.y, {rr, rt}, {rl, rb}};
        checks[2] = (struct CollisionCheck) {rr < c.pos.x && rb < c.pos.y, {rr, rb}, {rl, rt}};
        checks[3] = (struct CollisionCheck) {rl > c.pos.x && rb < c.pos.y, {rl, rb}, {rr, rt}};

        for(i = 0; i < 4; i++) {
                if(checks[i].check) {
                        dir = Vec2Sub(c.pos, checks[i].close);
                        dir = Vec2Normalize(dir);
                        rc = CollisionProjectPointOnAxis(checks[i].close, c.pos, dir);
                        rf = CollisionProjectPointOnAxis(checks[i].far, c.pos, dir);
                        len = CollisionCheckAxis(rc, rf, c.r, -1 * c.r);
                        disp = Vec2Mul(dir, len);
                        len = Vec2Length(disp);

                        if(len < EPSILON) {
                                return COLLISION_NONE;
                        } else if(len < minlen) {
                                minlen = len;
                                mindisp = disp;
                        }
                }
        }

        return (Collision) {true, mindisp};
}