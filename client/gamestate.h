#ifndef GAMESTATE_H
#define GAMESTATE_H

#define WORLD_MAX_X 255
#define WORLD_MAX_Y 255

enum GameObjectType {
        Player = 0,
        Bomb = 1
};

struct GameObject {
        float x;
        float y;
        enum GameObjectType type;
        unsigned char tint;
};

struct GameState {
        unsigned char worldX;
        unsigned char worldY;
        unsigned char world[WORLD_MAX_Y * WORLD_MAX_X];
        struct Hashmap* objects;

        int pingrequested;
        float timer;
};

#endif