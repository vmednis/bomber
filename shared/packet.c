#include <stdio.h>
#include <arpa/inet.h>
#include "../shared/packet.h"

/*Escape unescape*/
static int BufferEscape(unsigned char* buffer, int len);
static int BufferUnscape(unsigned char* buffer, int len);

/*Pack*/
static int PackPacketClientIdentify(unsigned char* buffer, void* packet);
static int PackPacketClientInput(unsigned char* buffer, void* packet);
static int PackPacketClientMessage(unsigned char* buffer, void* packet);

int PacketEncode(unsigned char* buffer, unsigned char type, void* packet) {
        unsigned char tmpBuffer[PACKET_MAX_BUFFER];
        int offset = 0;
        int ptr = 0;
        int len = 0;
        unsigned char check = 0;

        /*Packet beginning sequence*/
        buffer[offset + 0] = 0xff;
        buffer[offset + 1] = 0x00;

        /*Packet type*/
        offset = 2;
        buffer[offset + 0] = type;

        /*Packet data*/
        offset = 3;
        switch(type) {
                case PACKET_TYPE_CLIENT_IDENTIFY:
                        len = PackPacketClientIdentify(tmpBuffer, packet);
                        break;
                case PACKET_TYPE_CLIENT_INPUT:
                        len = PackPacketClientInput(tmpBuffer, packet);
                        break;
                case PACKET_TYPE_CLIENT_MESSAGE:
                        len = PackPacketClientMessage(tmpBuffer, packet);
                        break;
                default:
                        printf("Warning! Tried to decode unimplemented package %i", type);
        }
        len = BufferEscape(tmpBuffer, len);
        while(ptr < len) {
                buffer[offset + ptr] = tmpBuffer[ptr];
                ptr++;
        }
        offset += ptr;

        /*Checksum*/
        ptr = 0;
        while(ptr < offset) {
                check ^= buffer[ptr];
                ptr++;
        }
        buffer[offset] = check;

        /*Return the total length of the packet ready for net*/
        offset += 1;
        return offset;
}

void PacketDecode(unsigned char* buffer, int len, struct PacketCallbacks* callbacks) {
        /*Shutup unused*/
        buffer = buffer;
        len = len;
        callbacks = callbacks;
        BufferUnscape(buffer, len);
        return;
}

/*
** Escapes 0xffs in a buffer and returns the new length of the escaped data
*/
static int BufferEscape(unsigned char* buffer, int len) {
        unsigned char tmpBuffer[PACKET_MAX_BUFFER];
        int srcPtr = 0;
        int trgPtr = 0;

        while(srcPtr < len) {
                if(buffer[srcPtr] == 0xff) {
                        tmpBuffer[trgPtr] = 0xff;
                        tmpBuffer[trgPtr + 1] = 0xff;
                        trgPtr += 2;
                } else {
                        tmpBuffer[trgPtr] = buffer[srcPtr];
                        trgPtr++;
                }
                srcPtr++;
        }

        srcPtr = 0;
        while(srcPtr < trgPtr) {
                buffer[srcPtr] = tmpBuffer[srcPtr];
                srcPtr++;
        }

        buffer[trgPtr] = '\0'; /*Just to make debugging easier*/
        return trgPtr;
}

/*
** Unescapes 0xff 0xff sequences in the buffer and retuns the length of the
** unescaped data
*/
static int BufferUnscape(unsigned char* buffer, int len) {
        unsigned char tmpBuffer[PACKET_MAX_BUFFER];
        int srcPtr = 0;
        int trgPtr = 0;
        unsigned char lastByte = 0;

        while(srcPtr < len) {
                if(lastByte == 0xff && buffer[srcPtr] == 0xff) {
                        buffer[srcPtr] = 0;
                } else {
                        /*This will accept invalid escapes as just byte sequences*/
                        tmpBuffer[trgPtr] = buffer[srcPtr];
                        trgPtr++;
                }
                lastByte = buffer[srcPtr];
                srcPtr++;
        }

        srcPtr = 0;
        while(srcPtr < trgPtr) {
                buffer[srcPtr] = tmpBuffer[srcPtr];
                srcPtr++;
        }

        buffer[trgPtr] = '\0'; /*Just to make debugging easier*/
        return trgPtr;
}

static int PackPacketClientIdentify(unsigned char* buffer, void* packet) {
        struct PacketClientIdentify* pci = (struct PacketClientIdentify*)packet;
        int offset = 0;
        unsigned int ptr = 0;

        buffer[offset] = pci->protoVersion;
        offset += sizeof(pci->protoVersion);

        ptr = 0;
        while(ptr < sizeof(pci->playerName)) {
                buffer[offset + ptr] = pci->playerName[ptr];
                ptr++;
        }
        offset += sizeof(pci->playerName);

        buffer[offset] = pci->playerColor;
        offset += sizeof(pci->playerColor);

        return sizeof(struct PacketClientIdentify);
}

static int PackPacketClientInput(unsigned char* buffer, void* packet) {
        struct PacketClientInput* pci = (struct PacketClientInput*)packet;
        int offset = 0;

        buffer[offset] = pci->movementX;
        offset += sizeof(pci->movementX);

        buffer[offset] = pci->movementY;
        offset += sizeof(pci->movementY);

        buffer[offset] = pci->action;
        offset += sizeof(pci->action);

        return sizeof(struct PacketClientInput);
}

static int PackPacketClientMessage(unsigned char* buffer, void* packet) {
        struct PacketClientMessage* pcm = (struct PacketClientMessage*)packet;
        int offset = 0;
        unsigned int ptr = 0;

        while(ptr < sizeof(pcm->message)) {
                buffer[offset + ptr] = pcm->message[ptr];
                ptr++;
        }
        offset += sizeof(pcm->message);

        return sizeof(struct PacketClientMessage);
}