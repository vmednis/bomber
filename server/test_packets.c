#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../shared/packet.h"
#include "../shared/shrtest.h"

static void TestPackets();

int main() {
        TestPackets();
        return 0;
}

static void CallbackServerId(void* packet, void* data) {
        struct PacketServerId* psid = packet;
        psid = psid;
}

static void CallbacGameAreaInfo(void* packet, void* data) {
        struct PacketGameAreaInfo* pgai = packet;
        pgai = pgai;
}

static void CallbackMovableObjects(void* packet, void* data) {
        struct PacketMovableObjects* pmo = packet;
        pmo = pmo;
}

static void CallbackServerMessage(void* packet, void* data) {
        struct PacketServerMessage* psm = packet;
        psm = psm;
}

static void CallbackServerPlayers(void* packet, void* data) {
        struct PacketServerPlayers* psp = packet;
        psp = psp;
}

static void TestPackets() {
        unsigned char buffer[PACKET_MAX_BUFFER];
        struct PacketServerId psid;
        struct PacketGameAreaInfo pgai;
        struct PacketMovableObjects pmo;
        struct PacketServerMessage psm;
        struct PacketServerPlayers psp;
        struct PacketCallbacks pccbks;
        unsigned int len;
        unsigned char blocks[3] = { 0x04, 0x00, 0x01 };
        struct PacketMovableObjectInfo objects[3];
        struct PacketServerPlayerInfo players[3];

        pccbks.callback[PACKET_TYPE_SERVER_IDENTIFY] = &CallbackServerId;
        pccbks.callback[PACKET_TYPE_SERVER_GAME_AREA] = &CallbacGameAreaInfo;
        pccbks.callback[PACKET_TYPE_MOVABLE_OBJECTS] = &CallbackMovableObjects;
        pccbks.callback[PACKET_TYPE_SERVER_MESSAGE] = &CallbackServerMessage;
        pccbks.callback[PACKET_TYPE_SERVER_PLAYER_INFO] = &CallbackServerPlayers;

        psid.protoVersion = 0x80;
        psid.clientAccepted = 3;
        len = PacketEncode(buffer, PACKET_TYPE_SERVER_IDENTIFY, &psid);
        PacketDecode(buffer, len, &pccbks, NULL);

        pgai.sizeX = 100;
        pgai.sizeY = 100;
        memcpy(pgai.blockIDs, blocks, sizeof(blocks));
        len = PacketEncode(buffer, PACKET_TYPE_SERVER_GAME_AREA, &pgai);
        PacketDecode(buffer, len, &pccbks, NULL);

        objects[0].objectType = 0x01;
        objects[0].objectID = 1;
        objects[0].objectX = 2.5;
        objects[0].objectY = -2.5;
        objects[0].movement = 2;
        objects[0].status = 15;

        objects[1].objectType = 0x01;
        objects[1].objectID = 2;
        objects[1].objectX = -14.012;
        objects[1].objectY = 11.04;
        objects[1].movement = 3;
        objects[1].status = 4;

        /* objects[2].objectType = 0x01;
        objects[2].objectID = 3;
        objects[2].objectX = 12.012;
        objects[2].objectY = -3.04;
        objects[2].movement = 20;
        objects[2].status = 11; */

        pmo.objectCount = 2;
        memcpy(pmo.movableObjects, objects, sizeof(objects));
        len = PacketEncode(buffer, PACKET_TYPE_MOVABLE_OBJECTS, &pmo); /* 31 */
        PacketDecode(buffer, len, &pccbks, NULL);

        psm.messageType = 2;
        strcpy(psm.message, "Hello World!");
        len = PacketEncode(buffer, PACKET_TYPE_SERVER_MESSAGE, &psm);
        PacketDecode(buffer, len, &pccbks, NULL);

        players[0].playerID = 1;
        strcpy(players[0].playerName, "Liana");
        players[0].playerColor = 1;
        players[0].playerPoints = 1234;
        players[0].playerLives = 3;

        players[1].playerID = 2;
        strcpy(players[1].playerName, "Markuss");
        players[1].playerColor = 2;
        players[1].playerPoints = 3534;
        players[1].playerLives = 2;

        players[2].playerID = 3;
        strcpy(players[2].playerName, "Peteris");
        players[2].playerColor = 4;
        players[2].playerPoints = 746;
        players[2].playerLives = 1;

        psp.playerCount = 3;
        memcpy(psp.players, players, sizeof(players));
        len = PacketEncode(buffer, PACKET_TYPE_SERVER_PLAYER_INFO, &psp);
        PacketDecode(buffer, len, &pccbks, NULL);

        len = PacketEncode(buffer, PACKET_TYPE_SERVER_PING, NULL);
}