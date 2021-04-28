#include "../client/tests.h"
#include <stdio.h>
#include <string.h>
#include "../shared/packet.h"

static void CallbackClientId(void * packet, void* data) {
        struct PacketClientIdentify *pcid = packet;
        pcid = pcid;
        data = data;
}

static void CallbackClientInput(void * packet, void* data) {
        struct PacketClientInput *pcin = packet;
        pcin = pcin;
        data = data;
}

static void CallbackClientMessage(void * packet, void* data) {
        struct PacketClientMessage *pcmsg = packet;
        pcmsg = pcmsg;
        data = data;
}

static void TestPackets() {
        unsigned char buffer[PACKET_MAX_BUFFER];
        struct PacketClientId pcid;
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
        PacketDecode(buffer, len, &pccbks, NULL);

        pcin.movementX = -1;
        pcin.movementY = 127;
        pcin.action = 1;
        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_INPUT, &pcin);
        PacketDecode(buffer, len, &pccbks, NULL);

        strcpy(pcmsg.message, "Hello World!");
        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_MESSAGE, &pcmsg);
        PacketDecode(buffer, len, &pccbks, NULL);
}

void SelfTest() {
        TestPackets();
}