#include <stdio.h>
#include <string.h>
#include "../client/netcallbacks.h"
#include "../client/gamestate.h"
#include "../shared/packet.h"

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
        struct PacketMovableObjects* movable = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled MovableObj: count=%i\n", movable->objectCount);
}

void CallbackMessage(void * packet, void * passthrough) {
        struct PacketServerMessage* msg = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled ServerMsg: type=%i, data=%s\n", msg->messageType, msg->message);
}

void CallbackPlayerInfo(void * packet, void * passthrough) {
        struct PacketServerPlayerInfo* info = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled player info: id=%i, name=%s\n", info->playerID, info->playerName);
}

void CallbackPing(void * packet, void * passthrough) {
        struct PacketServerPing* ping = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled ping!\n");
}