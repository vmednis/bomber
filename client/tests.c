#include "../client/tests.h"
#include <stdio.h>
#include <string.h>
#include "../shared/packet.h"

static void CallbackClientId(void * packet) {
        struct PacketClientIdentify *pcid = packet;
        pcid = pcid;
}

static void CallbackClientInput(void * packet) {
        struct PacketClientInput *pcin = packet;
        pcin = pcin;
}

static void CallbackClientMessage(void * packet) {
        struct PacketClientMessage *pcmsg = packet;
        pcmsg = pcmsg;
}

static void TestPackets() {
        unsigned char buffer[PACKET_MAX_BUFFER];
        struct PacketClientIdentify pcid;
        struct PacketClientInput pcin;
        struct PacketClientMessage pcmsg;
        struct PacketCallbacks pccbks;
        unsigned int len;

        pccbks.callback[PACKET_TYPE_CLIENT_IDENTIFY] = &CallbackClientId;
        pccbks.callback[PACKET_TYPE_CLIENT_INPUT] = &CallbackClientInput;
        pccbks.callback[PACKET_TYPE_CLIENT_MESSAGE] = &CallbackClientMessage;

        pcid.protoVersion = 0x00;
        strcpy(pcid.playerName, "Valters");
        pcid.playerColor = 'x';
        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_IDENTIFY, &pcid);
        PacketDecode(buffer, len, &pccbks);

        pcin.movementX = -1;
        pcin.movementY = 127;
        pcin.action = 1;
        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_INPUT, &pcin);
        PacketDecode(buffer, len, &pccbks);

        strcpy(pcmsg.message, "Hello World!");
        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_MESSAGE, &pcmsg);
        PacketDecode(buffer, len, &pccbks);
}

void SelfTest() {
        TestPackets();
}