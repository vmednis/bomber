#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../client/netcallbacks.h"
#include "../client/gamestate.h"
#include "../client/hashmap.h"
#include "../shared/packet.h"

#define UNUSED __attribute__((unused))

void CallbackServerId(void * packet, void * passthrough) {
        struct PacketServerId* serverId = packet;
        struct GameState* gameState = passthrough;

        if (serverId->protoVersion == 0x0 && serverId->clientAccepted) {
                /* TODO: Switch some internal states or something*/
                gameState = gameState;
        }
}

void CallbackGameArea(void * packet, void * passthrough) {
        struct PacketGameAreaInfo* gameArea = packet;
        struct GameState* gameState = passthrough;

        gameState->worldX = gameArea->sizeX;
        gameState->worldY = gameArea->sizeY;
        memcpy(gameState->world, gameArea->blockIDs, gameArea->sizeY * gameArea->sizeX);
}

void CallbackMovableObj(void * packet, void * passthrough) {
        struct GameState* gameState = passthrough;
        struct PacketMovableObjects* movable = packet;
        struct PacketMovableObjectInfo info;
        struct GameObject* obj;
        struct HashmapIterator* iter;
        unsigned int objsToRemove[4096];
        unsigned int i = 0;
        unsigned int len = 0;

        /* Mark all the old ones*/
        iter = HashmapIterator(gameState->objects);
        while((obj = HashmapNext(iter))) {
                obj->remove = 1;
        }

        /* Updates/Inserts */
        while (i < movable->objectCount) {
                info = movable->movableObjects[i];
                obj = HashmapGet(gameState->objects, info.objectID);
                if(obj == NULL) {
                        obj = malloc(sizeof(struct GameObject));
                        HashmapPut(gameState->objects, info.objectID, obj);
                }
                obj->id = info.objectID;
                obj->type = info.objectType;
                obj->x = info.objectX;
                obj->y = info.objectY;
                obj->remove = 0;
                i++;
        }

        /* Remove the ones that weren't updated */
        len = 0;
        iter = HashmapIterator(gameState->objects);
        while((obj = HashmapNext(iter))) {
                if(obj->remove) {
                        objsToRemove[len] = obj->id;
                        len++;
                }
        }
        i = 0;
        while(i < len) {
                HashmapRemove(gameState->objects, objsToRemove[i]);
                i++;
        }
}

void CallbackMessage(void * packet, void * passthrough) {
        struct PacketServerMessage* msg = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled ServerMsg: type=%i, data=%s\n", msg->messageType, msg->message);
}

void CallbackServerPlayers(void * packet, void * passthrough) {
        struct PacketServerPlayers* players = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled player info: count=%i\n", players->playerCount);
}

void CallbackPing(UNUSED void * packet, void * passthrough) {
        struct GameState* gameState = passthrough;
        gameState->pingrequested = 1;
}