#ifndef PACKET_H
#define PACKET_H

#define PACKET_MAX_BUFFER 1024*1024

#define PACKET_TYPE_CLIENT_IDENTIFY 0x00
#define PACKET_TYPE_CLIENT_INPUT 0x01
#define PACKET_TYPE_CLIENT_MESSAGE 0x02

#define PACKET_ERR_ENCODE_UNIMPLEMENTED -1
#define PACKET_ERR_DECODE_START -1
#define PACKET_ERR_DECODE_CHECKSUM -2
#define PACKET_ERR_DECODE_OTHER -3
#define PACKET_ERR_DECODE_UNIMPLEMENTED -4

/*Client Packets*/
struct PacketClientIdentify {
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
struct PacketServerIdentification {
        unsigned char protoVersion;
        unsigned int clientAccepted;
};

struct PacketGameAreaInfo {
        unsigned char sizeX;
        unsigned char sizeY;
        unsigned char* blockIDs;
};

struct PacketMovableObjectInfo {
        unsigned char objectType;
        unsigned int objectID;
        float objectX;
        float objectY;
        char movement;
};

struct PacketMovableObjects {
        unsigned char objectCount;
        struct PacketMovableObjectInfo *movableObjects;
};

struct PacketServerMessage {
        char messageType;
        char message[256];
};

struct PacketServerPlayerInfo {
        unsigned int playerCount;
        unsigned int playrID;
        char playerName[32];
        char playerColor;
        unsigned int playerPoints;
        unsigned char playerLives;
};

/*Implementation*/
struct PacketCallbacks {
        void (*callback[256])(void*, void*);
};

int PacketEncode(unsigned char* buffer, unsigned char type, void* packet);
int PacketDecode(unsigned char* buffer, int len, struct PacketCallbacks* callbacks, void* callbackData);

#endif