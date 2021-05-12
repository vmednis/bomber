#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <raylib.h>

#define WORLD_MAX_X 255
#define WORLD_MAX_Y 255

enum GameObjectType {
        Player = 0,
        Bomb = 1
};

struct GameObject {
        unsigned int id;
        float x;
        float y;
        enum GameObjectType type;
        unsigned char tint;
        int remove;
};

struct PlayerInfo {
        unsigned int id;
        char name[256];
        unsigned char color;
        int remove;
};

struct GameState {
        unsigned char worldX;
        unsigned char worldY;
        unsigned char world[WORLD_MAX_Y * WORLD_MAX_X];
        unsigned int playerId;
        struct Hashmap* objects;
        struct Hashmap* players;
        Camera2D camera;

        int pingrequested;
        float timer;
};

#endif