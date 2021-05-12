#ifndef BOMBSERV_H
#define BOMBSERV_H

#define MAX_OBJECTS 1024

typedef struct server {
        int address;
        int port;
        int fd;
} server;

enum GameObjectType {
        Player = 0,
        Bomb = 1
};

struct GameObjectPlayer {
        int fd;
        char name[32];
        unsigned char color;
};

struct GameObjectBomb {
        float timeToDetonation;
};

struct GameObject {
        int active;
        enum GameObjectType type;
        float x;
        float y;
        float velx;
        float vely;
        union {
                struct GameObjectPlayer player;
                struct GameObjectBomb bomb;
        } extra;
};

struct GameState {
        struct GameObject objects[MAX_OBJECTS];
        unsigned int worldX;
        unsigned int worldY;
        unsigned char world[MAX_BLOCK_IDS];
};

struct SourcedGameState {
        unsigned int playerId;
        struct GameState* gameState;
};

/* Default game arena */
unsigned char blocks[169] = {
                                1,1,1,1,1,1,1,1,1,1,1,1,1,
                                1,0,0,2,2,2,2,2,2,2,2,2,1,
                                1,0,1,2,1,2,1,2,1,2,1,2,1,
                                1,2,2,2,2,2,2,2,2,2,2,2,1,
                                1,2,1,2,1,2,1,2,1,2,1,2,1,
                                1,2,2,2,2,2,2,2,2,2,2,2,1,
                                1,2,1,2,1,2,1,2,1,2,1,2,1,
                                1,2,2,2,2,2,2,2,2,2,2,2,1,
                                1,2,1,2,1,2,1,2,1,2,1,2,1,
                                1,2,2,2,2,2,2,2,2,2,2,2,1,
                                1,2,1,2,1,2,1,2,1,2,1,0,1,
                                1,2,2,2,2,2,2,2,2,2,0,0,1,
                                1,1,1,1,1,1,1,1,1,1,1,1,1
};

#define PACKET_BUFFER_PICK(buffer, offset, type, value, converter) do { \
        value = converter(*(type *)((void *) (buffer + offset))); \
} while(0);

#endif