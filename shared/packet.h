#ifndef PACKET_H
#define PACKET_H

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

/*Implementation*/
struct PacketCallbacks {
        void (*callback[256])(void *);
};

int PacketEncode(char * buffer, unsigned char type, void * packet);
void PacketDecode(char * buffer, int len, struct PacketCallbacks * callbacks);

#endif