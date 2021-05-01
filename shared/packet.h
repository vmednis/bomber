#ifndef PACKET_H
#define PACKET_H

#define PACKET_MAX_BUFFER 1024*1024
#define MAX_BLOCK_IDS (256*256)
#define MAX_CLIENTS 256
#define MAX_MOVABLE_OBJECTS 1024

#define PACKET_TYPE_CLIENT_IDENTIFY 0x00
#define PACKET_TYPE_CLIENT_INPUT 0x01
#define PACKET_TYPE_CLIENT_MESSAGE 0x02
#define PACKET_TYPE_CLIENT_PING_ANSWER 0x03
#define PACKET_TYPE_SERVER_IDENTIFY 0x80
#define PACKET_TYPE_SERVER_GAME_AREA 0x81
#define PACKET_TYPE_MOVABLE_OBJECTS 0x82
#define PACKET_TYPE_SERVER_MESSAGE 0x83
#define PACKET_TYPE_SERVER_PLAYER_INFO 0x84
#define PACKET_TYPE_SERVER_PING 0x85

#define PACKET_ERR_ENCODE_UNIMPLEMENTED -1
#define PACKET_ERR_DECODE_START -1
#define PACKET_ERR_DECODE_CHECKSUM -2
#define PACKET_ERR_DECODE_OTHER -3
#define PACKET_ERR_DECODE_UNIMPLEMENTED -4

/*Client Packets*/
struct PacketClientId {
        unsigned char protoVersion;
        char playerName[32];
        char playerColor;
};

struct PacketClientInput {
        char movementX;
        char movementY;
        unsigned char action;
};

struct PacketClientMessage {
        char message[256];
};

/*Server Packets*/
struct PacketServerId {
        unsigned char protoVersion;
        unsigned int clientAccepted;
};

struct PacketGameAreaInfo {
        unsigned char sizeX;
        unsigned char sizeY;
        unsigned char blockIDs[MAX_BLOCK_IDS];
};

struct PacketMovableObjectInfo {
        unsigned char objectType;
        unsigned int objectID;
        float objectX;
        float objectY;
        char movement;
        unsigned char status;
};

struct PacketMovableObjects {
        unsigned char objectCount;
        struct PacketMovableObjectInfo movableObjects[MAX_MOVABLE_OBJECTS * sizeof(struct PacketMovableObjectInfo)];
};

struct PacketServerMessage {
        char messageType;
        char message[256];
};

struct PacketServerPlayerInfo {
        unsigned int playerID;
        char playerName[32];
        char playerColor;
        unsigned int playerPoints;
        unsigned char playerLives;
};

struct PacketServerPlayers {
        unsigned int playerCount;
        struct PacketServerPlayerInfo players[MAX_CLIENTS * sizeof(struct PacketServerPlayerInfo)];
};

/*Implementation*/
struct PacketCallbacks {
        void (*callback[256])(void*, void*);
};

int PacketEncode(unsigned char* buffer, unsigned char type, void* packet);
int PacketDecode(unsigned char* buffer, int len, struct PacketCallbacks* callbacks, void* callbackData);

#endif