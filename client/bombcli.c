#include <stdio.h>
#include <string.h>
#include "../shared/packet.h"
#include "../shared/shrtest.h"

void testPackets();

int main() {
        hellotest("Client");
        testPackets();
        return 0;
}

void testPackets() {
        unsigned char buffer[PACKET_MAX_BUFFER];
        struct PacketClientIdentify pcid;
        struct PacketClientInput pcin;
        struct PacketClientMessage pcmsg;
        unsigned int len;

        pcid.protoVersion = 0x00;
        strcpy(pcid.playerName, "Valters");
        pcid.playerColor = 'x';
        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_IDENTIFY, &pcid);
        len = len;

        pcin.movementX = -1;
        pcin.movementY = 127;
        pcin.action = 1;
        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_INPUT, &pcin);
        len = len;

        strcpy(pcmsg.message, "Hello World!");
        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_MESSAGE, &pcmsg);
        len = len;
}
