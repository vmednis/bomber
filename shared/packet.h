#ifndef PACKET_H
#define PACKET_H

#define PACKET_MAX_BUFFER 1024*1024

int EscapeBuffer(unsigned char* buffer, int len);
int UnescapeBuffer(unsigned char* buffer, int len);

/*Client Packets*/
struct PacketClientIdentify {
        unsigned char protoVersion;
        char playerName[32];
        char playerColor[32];
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

struct PacketPlayerInfo {
        unsigned int playerCount;
        unsigned int playrID;
        char playerName[32];
        char playerColor;
        unsigned int playerPoints;
        unsigned char playerLives;
};

/*Implementation*/
struct PacketCallbacks {
        void (*callback[256])(void*);
};

#endif