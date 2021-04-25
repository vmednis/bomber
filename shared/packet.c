#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "../shared/packet.h"

/*Escape unescape*/
static int BufferEscape(unsigned char* buffer, int len);
static int BufferUnscape(unsigned char* buffer, int len);

/*Pack*/
static int PackPacketClientIdentify(unsigned char* buffer, void* packet);
static int PackPacketClientInput(unsigned char* buffer, void* packet);
static int PackPacketClientMessage(unsigned char* buffer, void* packet);
static int PackPacketServerIdentify(unsigned char* buffer, void* packet);
static int PackPacketServerGameAreaInfo(unsigned char* buffer, void* packet);

/*Unpack*/
static void UnpackPacketClientIdentify(unsigned char* buffer, void* packet);
static void UnpackPacketClientInput(unsigned char* buffer, void* packet);
static void UnpackPacketClientMessage(unsigned char* buffer, void* packet);
static void UnpackPacketServerIdentify(unsigned char* buffer, void* packet);
static void UnpackPacketServerGameAreaInfo(unsigned char* buffer, void* packet);

/*Helper macros for working with buffers where byte precission is needed*/
#define PACKET_BUFFER_PLACE(buffer, offset, type, value, converter) do { \
        *(type *)((void *) (buffer + offset)) = converter(value); \
} while(0);

#define PACKET_BUFFER_PICK(buffer, offset, type, value, converter) do { \
        value = converter(*(type *)((void *) (buffer + offset))); \
} while(0);

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
                case PACKET_TYPE_SERVER_IDENTIFY:
                        len = PackPacketServerIdentify(tmpBuffer, packet);
                        break;
                case PACKET_TYPE_SERVER_GAME_AREA:
                        len = PackPacketServerGameAreaInfo(tmpBuffer, packet);
                        break;
                default:
                        printf("Warning! Tried to encode unimplemented package %i", type);
                        return PACKET_ERR_ENCODE_UNIMPLEMENTED;
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

int PacketDecode(unsigned char* buffer, int len, struct PacketCallbacks* callbacks) {
        int ptr = 0;
        unsigned char check = 0;
        unsigned char type;
        unsigned char packet[PACKET_MAX_BUFFER];

        /* Check for packet start */
        if(!(buffer[0] == 0xff && buffer[1] == 0x00)) {
                return PACKET_ERR_DECODE_START;
        }

        /*Check if packet is of minimum length*/
        if(len < 4) {
                return PACKET_ERR_DECODE_OTHER;
        }

        /* Check the checksum */
        while (ptr < len - 1) {
                check ^= buffer[ptr];
                ptr++;
        }
        if(buffer[len - 1] != check) {
                return PACKET_ERR_DECODE_CHECKSUM;
        }

        /* Get the type */
        type = buffer[2];

        /* Unescape and then unpack */
        len = BufferUnscape(buffer + 3, len - 4);
        switch(type) {
                case PACKET_TYPE_CLIENT_IDENTIFY:
                        UnpackPacketClientIdentify(buffer + 3, packet);
                        break;
                case PACKET_TYPE_CLIENT_INPUT:
                        UnpackPacketClientInput(buffer + 3, packet);
                        break;
                case PACKET_TYPE_CLIENT_MESSAGE:
                        UnpackPacketClientMessage(buffer + 3, packet);
                        break;
                case PACKET_TYPE_SERVER_IDENTIFY:
                        UnpackPacketServerIdentify(buffer + 3, packet);
                        break;
                case PACKET_TYPE_SERVER_GAME_AREA:
                        UnpackPacketServerGameAreaInfo(buffer + 3, packet);
                        break;
                default:
                        printf("Warning! Tried to decode unimplemented package %i", type);
                        return PACKET_ERR_DECODE_UNIMPLEMENTED;

        }

        callbacks->callback[type](packet);
        return 0;
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
        struct PacketClientId* pci = (struct PacketClientId*)packet;
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

        return sizeof(struct PacketClientId);
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

static int PackPacketServerIdentify(unsigned char* buffer, void* packet) {
        struct PacketServerId* psi = (struct PacketServerId*)packet;
        int offset = 0;

        buffer[offset] = psi->protoVersion;
        offset += sizeof(psi->protoVersion);

        PACKET_BUFFER_PLACE(buffer, offset, unsigned int, psi->clientAccepted,htonl);
        offset += sizeof(psi->clientAccepted);

        return sizeof(struct PacketServerId);
}

static int PackPacketServerGameAreaInfo(unsigned char* buffer, void* packet) {
        struct PacketGameAreaInfo* pgai = (struct PacketGameAreaInfo*)packet;
        int ptr = 0;
        int offset = 0;

        buffer[offset] = pgai->sizeX;
        offset += sizeof(pgai->sizeX);

        buffer[offset] = pgai->sizeY;
        offset += sizeof(pgai->sizeY);

        while(ptr < pgai->sizeX * pgai->sizeY) {
                buffer[offset + ptr] = pgai->blockIDs[ptr];
                ptr++;
        }
        offset += pgai->sizeX * pgai->sizeY * sizeof(pgai->blockIDs[0]);

        return offset;
}


static void UnpackPacketClientIdentify(unsigned char* buffer, void* packet) {
        struct PacketClientId* pci = (struct PacketClientId*)packet;
        int offset = 0;
        unsigned int ptr = 0;

        pci->protoVersion = buffer[offset];
        offset += sizeof(pci->protoVersion);

        while(ptr < sizeof(pci->playerName)) {
                pci->playerName[ptr] = buffer[offset + ptr];
                ptr++;
        }
        offset += sizeof(pci->playerName);

        pci->playerColor = buffer[offset];
        offset += sizeof(pci->playerColor);
}

static void UnpackPacketClientInput(unsigned char* buffer, void* packet) {
        struct PacketClientInput* pci = (struct PacketClientInput*)packet;
        int offset = 0;

        pci->movementX = buffer[offset];
        offset += sizeof(pci->movementX);

        pci->movementY = buffer[offset];
        offset += sizeof(pci->movementY);

        pci->action = buffer[offset];
        offset += sizeof(pci->action);
}

static void UnpackPacketClientMessage(unsigned char* buffer, void* packet) {
        struct PacketClientMessage* pcm = (struct PacketClientMessage*)packet;
        int offset = 0;
        unsigned int ptr = 0;

        while(ptr < sizeof(pcm->message)) {
                pcm->message[ptr] = buffer[offset + ptr];
                ptr++;
        }
        offset += sizeof(pcm->message);
}

static void UnpackPacketServerIdentify(unsigned char* buffer, void* packet) {
        struct PacketServerId* psi = (struct PacketServerId*)packet;
        int offset = 0;

        psi->protoVersion = buffer[offset];
        offset += sizeof(psi->protoVersion);

        /*psi->clientAccepted = ntohl(buffer[offset]);*/
        PACKET_BUFFER_PICK(buffer, offset, unsigned int, psi->clientAccepted, ntohl);
        offset += sizeof(psi->clientAccepted);
}

static void UnpackPacketServerGameAreaInfo(unsigned char* buffer, void* packet) {
        struct PacketGameAreaInfo* pgai = (struct PacketGameAreaInfo*)packet;
        int ptr = 0;
        int offset = 0;

        pgai->sizeX = buffer[offset];
        offset += sizeof(pgai->sizeX);

        pgai->sizeY = buffer[offset];
        offset += sizeof(pgai->sizeY);

        /* I really don't like this malloc, maybe we should flatten the struct */
        pgai->blockIDs = malloc(pgai->sizeX * pgai->sizeY * sizeof(unsigned char));
        while(ptr < pgai->sizeX * pgai->sizeY) {
                pgai->blockIDs[ptr] = buffer[offset + ptr];
        }
        offset += pgai->sizeX * pgai->sizeY * sizeof(unsigned char);
}