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
                gameState->playerId = serverId->clientAccepted;
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
        struct PlayerInfo* player;
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
                        obj->x = info.objectX;
                        obj->y = info.objectY;
                }

                player = HashmapGet(gameState->players, info.objectID);
                obj->id = info.objectID;
                obj->type = info.objectType;
                obj->prevx = obj->x;
                obj->prevy = obj->y;
                obj->x = info.objectX;
                obj->y = info.objectY;
                if(player) obj->tint = player->color;
                obj->remove = 0;
                i++;
        }

        /* Remove the ones that weren't updated */
        len = 0;
        iter = HashmapIterator(gameState->objects);
        while((obj = HashmapNext(iter))) {
                if(obj->remove) {
                        objsToRemove[len] = obj->id;
                        free(obj);
                        len++;
                }
        }
        i = 0;
        while(i < len) {
                HashmapRemove(gameState->objects, objsToRemove[i]);
                i++;
        }

        /* Update game obj interpolation timers */
        gameState->objUpdateLen = gameState->timerObjUpdate;
        gameState->timerObjUpdate = 0.00;
}

void CallbackMessage(void * packet, void * passthrough) {
        struct PacketServerMessage* msg = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled ServerMsg: type=%i, data=%s\n", msg->messageType, msg->message);
}

void CallbackServerPlayers(void * packet, void * passthrough) {
        struct GameState* gameState = passthrough;
        struct PacketServerPlayers* players = packet;
        struct PacketServerPlayerInfo info;
        struct PlayerInfo *player;
        struct HashmapIterator* iter;
        unsigned int playersToRemove[4096];
        unsigned int i = 0;
        unsigned int len = 0;

        /* Mark all the old ones*/
        iter = HashmapIterator(gameState->players);
        while((player = HashmapNext(iter))) {
                player->remove = 1;
        }

        /* Insert/update */
        while(i < players->playerCount) {
                info = players->players[i];
                player = HashmapGet(gameState->players, info.playerID);
                if(player == NULL) {
                        player = malloc(sizeof(struct PlayerInfo));
                        HashmapPut(gameState->players, info.playerID, player);
                }
                player->id = info.playerID;
                player->color = info.playerColor;
                strcpy(player->name, info.playerName);
                player->remove = 0;
                i++;
        }

        /* Remove the ones that weren't updated */
        len = 0;
        iter = HashmapIterator(gameState->players);
        while((player = HashmapNext(iter))) {
                if(player->remove) {
                        playersToRemove[len] = player->id;
                        free(player);
                        len++;
                }
        }
        i = 0;
        while(i < len) {
                HashmapRemove(gameState->players, playersToRemove[i]);
                i++;
        }
}

void CallbackPing(UNUSED void * packet, void * passthrough) {
        struct GameState* gameState = passthrough;
        gameState->pingrequested = 1;
}