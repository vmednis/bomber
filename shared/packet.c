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
static int PackPacketMovableObjects(unsigned char* buffer, void* packet);
static int PackPacketServerMessage(unsigned char* buffer, void* packet);
static int PackPacketServerPlayerInfo(unsigned char* buffer, void* packet);

/*Unpack*/
static void UnpackPacketClientIdentify(unsigned char* buffer, void* packet);
static void UnpackPacketClientInput(unsigned char* buffer, void* packet);
static void UnpackPacketClientMessage(unsigned char* buffer, void* packet);
static void UnpackPacketServerIdentify(unsigned char* buffer, void* packet);
static void UnpackPacketServerGameAreaInfo(unsigned char* buffer, void* packet);
static void UnpackPacketMovableObjects(unsigned char* buffer, void* packet);
static void UnpackPacketServerMessage(unsigned char* buffer, void* packet);
static void UnpackPacketServerPlayerInfo(unsigned char* buffer, void* packet);

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
        switch (type) {
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
        case PACKET_TYPE_MOVABLE_OBJECTS:
                len = PackPacketMovableObjects(tmpBuffer, packet);
                break;
        case PACKET_TYPE_SERVER_MESSAGE:
                len = PackPacketServerMessage(tmpBuffer, packet);
                break;
        case PACKET_TYPE_SERVER_PLAYER_INFO:
                len = PackPacketServerPlayerInfo(tmpBuffer, packet);
                break;
        default:
                printf("Warning! Tried to encode unimplemented package %i", type);
                return PACKET_ERR_ENCODE_UNIMPLEMENTED;
        }
        len = BufferEscape(tmpBuffer, len);
        while (ptr < len) {
                buffer[offset + ptr] = tmpBuffer[ptr];
                ptr++;
        }
        offset += ptr;

        /*Checksum*/
        ptr = 0;
        while (ptr < offset) {
                check ^= buffer[ptr];
                ptr++;
        }
        buffer[offset] = check;

        /*Return the total length of the packet ready for net*/
        offset += 1;
        return offset;
}


int PacketDecode(unsigned char* buffer, int len, struct PacketCallbacks* callbacks, void* callbackData) {
        int ptr = 0;
        unsigned char check = 0;
        unsigned char type;
        unsigned char packet[PACKET_MAX_BUFFER];

        /* Check for packet start */
        if (!(buffer[0] == 0xff && buffer[1] == 0x00)) {
                return PACKET_ERR_DECODE_START;
        }

        /*Check if packet is of minimum length*/
        if (len < 4) {
                return PACKET_ERR_DECODE_OTHER;
        }

        /* Check the checksum */
        while (ptr < len - 1) {
                check ^= buffer[ptr];
                ptr++;
        }
        if (buffer[len - 1] != check) {
                return PACKET_ERR_DECODE_CHECKSUM;
        }

        /* Get the type */
        type = buffer[2];

        /* Unescape and then unpack */
        len = BufferUnscape(buffer + 3, len - 4);
        switch (type) {
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
        case PACKET_TYPE_MOVABLE_OBJECTS:
                UnpackPacketMovableObjects(buffer + 3, packet);
                break;
        case PACKET_TYPE_SERVER_MESSAGE:
                UnpackPacketServerMessage(buffer + 3, packet);
                break;
        case PACKET_TYPE_SERVER_PLAYER_INFO:
                UnpackPacketServerPlayerInfo(buffer + 3, packet);
                break;
        default:
                printf("Warning! Tried to decode unimplemented package %i", type);
                return PACKET_ERR_DECODE_UNIMPLEMENTED;
        }

        callbacks->callback[type](packet, callbackData);
        return 0;
}

/*
** Escapes 0xffs in a buffer and returns the new length of the escaped data
*/
static int BufferEscape(unsigned char* buffer, int len) {
        unsigned char tmpBuffer[PACKET_MAX_BUFFER];
        int srcPtr = 0;
        int trgPtr = 0;

        while (srcPtr < len) {
                if (buffer[srcPtr] == 0xff) {
                        tmpBuffer[trgPtr] = 0xff;
                        tmpBuffer[trgPtr + 1] = 0xff;
                        trgPtr += 2;
                }
                else {
                        tmpBuffer[trgPtr] = buffer[srcPtr];
                        trgPtr++;
                }
                srcPtr++;
        }

        srcPtr = 0;
        while (srcPtr < trgPtr) {
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

        while (srcPtr < len) {
                if (lastByte == 0xff && buffer[srcPtr] == 0xff) {
                        buffer[srcPtr] = 0;
                }
                else {
                        /*This will accept invalid escapes as just byte sequences*/
                        tmpBuffer[trgPtr] = buffer[srcPtr];
                        trgPtr++;
                }
                lastByte = buffer[srcPtr];
                srcPtr++;
        }

        srcPtr = 0;
        while (srcPtr < trgPtr) {
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
        while (ptr < sizeof(pci->playerName)) {
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

        while (ptr < sizeof(pcm->message)) {
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

        PACKET_BUFFER_PLACE(buffer, offset, unsigned int, psi->clientAccepted, htonl);
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

        while (ptr < pgai->sizeX * pgai->sizeY) {
                buffer[offset + ptr] = pgai->blockIDs[ptr];
                ptr++;
        }
        offset += pgai->sizeX * pgai->sizeY * sizeof(pgai->blockIDs[0]);

        return offset;
}

static int PackPacketMovableObjects(unsigned char* buffer, void* packet) {
        struct PacketMovableObjects* pmo = (struct PacketMovableObjects*)packet;
        int offset = 0;
        int ptr = 0;
        int tmp[3], i = 0;

        buffer[offset] = pmo->objectCount;
        offset += sizeof(pmo->objectCount);

        while (i < pmo->objectCount) {
                buffer[offset] = pmo->movableObjects[i].objectType;
                offset += sizeof(pmo->movableObjects[i].objectType);

                PACKET_BUFFER_PLACE(buffer, offset, unsigned int, pmo->movableObjects[i].objectID, htonl);
                offset += sizeof(pmo->movableObjects[i].objectID);

                tmp[2] = *(int*)&(pmo->movableObjects[i].objectX);
                PACKET_BUFFER_PLACE(buffer, offset, unsigned int, tmp[2], htonl);
                offset += sizeof(pmo->movableObjects[i].objectX);

                tmp[2] = *(int*)&(pmo->movableObjects[i].objectY);
                PACKET_BUFFER_PLACE(buffer, offset, unsigned int, tmp[2], htonl);
                offset += sizeof(pmo->movableObjects[i].objectY);

                buffer[offset] = pmo->movableObjects[i].movement;
                offset += sizeof(pmo->movableObjects[i].movement);

                buffer[offset] = pmo->movableObjects[i].status;
                offset += sizeof(pmo->movableObjects[i].status);

                ptr += sizeof(pmo->movableObjects[i]);
                i++;
        }
        return offset;
}

static int PackPacketServerMessage(unsigned char* buffer, void* packet) {
        struct PacketServerMessage* psm = (struct PacketServerMessage*)packet;
        int offset = 0;
        int ptr = 0;

        buffer[offset] = psm->messageType;
        offset += sizeof(psm->messageType);

        while (ptr < sizeof(psm->message)) {
                buffer[offset + ptr] = psm->message[ptr];
                ptr++;
        }
        offset += sizeof(psm->message);

        return sizeof(struct PacketServerMessage);
}

static int PackPacketServerPlayerInfo(unsigned char* buffer, void* packet) {
        struct PacketServerPlayers* psp = (struct PacketServerPlayers*)packet;
        int offset = 0;
        int ptr = 0, ptr2;
        int i = 0;

        buffer[offset] = psp->playerCount;
        offset += sizeof(psp->playerCount);

        while (i < psp->playerCount) {
                PACKET_BUFFER_PLACE(buffer, offset, unsigned int, psp->players[i].playerID, htonl);
                offset += sizeof(psp->players[i].playerID);

                ptr2 = 0;
                while (ptr2 < sizeof(psp->players->playerName)) {
                        buffer[offset + ptr2] = psp->players[i].playerName[ptr2];
                        ptr2++;
                }
                offset += sizeof(psp->players->playerName);

                buffer[offset] = psp->players[i].playerColor;
                offset += sizeof(psp->players[i].playerColor);

                PACKET_BUFFER_PLACE(buffer, offset, unsigned int, psp->players[i].playerPoints, htonl);
                offset += sizeof(psp->players[i].playerPoints);

                buffer[offset] = psp->players[i].playerLives;
                offset += sizeof(psp->players[i].playerLives);

                ptr += sizeof(psp->players[i]);
                i++;
        }
        offset += sizeof(psp->players);
        return offset;
}

static void UnpackPacketClientIdentify(unsigned char* buffer, void* packet) {
        struct PacketClientId* pci = (struct PacketClientId*)packet;
        int offset = 0;
        unsigned int ptr = 0;

        pci->protoVersion = buffer[offset];
        offset += sizeof(pci->protoVersion);

        while (ptr < sizeof(pci->playerName)) {
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

        while (ptr < sizeof(pcm->message)) {
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

        pgai->blockIDs[pgai->sizeX * pgai->sizeY] = buffer[offset];
        while (ptr < pgai->sizeX * pgai->sizeY) {
                pgai->blockIDs[ptr] = buffer[offset + ptr];
                ptr += sizeof(pgai->blockIDs[0]);
        }
        offset += pgai->sizeX * pgai->sizeY;
}

static void UnpackPacketMovableObjects(unsigned char* buffer, void* packet) {
        struct PacketMovableObjects* pmo = (struct PacketMovableObjects*)packet;
        int offset = 0;
        int ptr = 0;
        int x, y, i = 0;

        pmo->objectCount = buffer[offset];
        offset += sizeof(pmo->objectCount);

        while (i < pmo->objectCount) {
                pmo->movableObjects[i].objectType = buffer[offset];
                offset += sizeof(pmo->movableObjects->objectType);

                PACKET_BUFFER_PICK(buffer, offset, unsigned int, pmo->movableObjects[i].objectID, ntohl);
                offset += sizeof(pmo->movableObjects->objectID);

                PACKET_BUFFER_PICK(buffer, offset, unsigned int, x, ntohl);
                pmo->movableObjects[i].objectX = *(float*)&x;
                offset += sizeof(pmo->movableObjects->objectX);

                PACKET_BUFFER_PICK(buffer, offset, unsigned int, y, ntohl);
                pmo->movableObjects[i].objectY = *(float*)&y;
                offset += sizeof(pmo->movableObjects->objectY);

                pmo->movableObjects[i].movement = buffer[offset];
                offset += sizeof(pmo->movableObjects->movement);

                pmo->movableObjects[i].status = buffer[offset];
                offset += sizeof(pmo->movableObjects->status);

                ptr += sizeof(pmo->movableObjects[0]);
                i++;
        }
}

static void UnpackPacketServerMessage(unsigned char* buffer, void* packet) {
        struct PacketServerMessage* psm = (struct PacketServerMessage*)packet;
        int offset = 0;
        unsigned int ptr = 0;

        psm->messageType = buffer[offset];
        offset += sizeof(psm->messageType);

        while (ptr < sizeof(psm->message)) {
                psm->message[ptr] = buffer[offset + ptr];
                ptr++;
        }
        offset += sizeof(psm->message);
}

static void UnpackPacketServerPlayerInfo(unsigned char* buffer, void* packet) {
        struct PacketServerPlayers* psp = (struct PacketServerPlayers*)packet;
        int offset = 0;
        int ptr = 0, ptr2, i = 0;

        psp->playerCount = buffer[offset];
        offset += sizeof(psp->playerCount);
        offset = offset;

        while (i < psp->playerCount) {
                PACKET_BUFFER_PICK(buffer, offset, unsigned int, psp->players[i].playerID, ntohl);
                offset += sizeof(psp->players->playerID);

                ptr2 = 0;
                while (ptr2 < sizeof(psp->players->playerName)) {
                        psp->players[i].playerName[ptr2] = buffer[offset + ptr2];
                        ptr2++;
                }
                offset += sizeof(psp->players->playerName);

                psp->players[i].playerColor = buffer[offset];
                offset += sizeof(psp->players->playerColor);

                PACKET_BUFFER_PICK(buffer, offset, unsigned int, psp->players[i].playerPoints, ntohl);
                offset += sizeof(psp->players[i].playerPoints);

                psp->players[i].playerLives = buffer[offset];
                offset += sizeof(psp->players->playerLives);

                ptr += sizeof(psp->players[i]);
                i++;
        }
        offset += sizeof(psp->players);
}