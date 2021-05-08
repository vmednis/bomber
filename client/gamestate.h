#ifndef GAMESTATE_H
#define GAMESTATE_H

#define WORLD_MAX_X 255
#define WORLD_MAX_Y 255

struct GameState {
        unsigned char worldX;
        unsigned char worldY;
        unsigned char world[WORLD_MAX_Y * WORLD_MAX_X];

        float timer;
};

#endif