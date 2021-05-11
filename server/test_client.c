#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "../shared/packet.h"
#include "../client/tests.h"
#include "bombserv.h"

#define HOST "127.0.0.1"
#define PORT 3001

static void CallbackServerId(void* packet, void* data) {
        struct PacketServerId* psid = packet;
        printf("protocol version = %u, client accepted = %u\n", psid->protoVersion, psid->clientAccepted);
}

static void CallbacGameAreaInfo(void* packet, void* data) {
        int i;
        struct PacketGameAreaInfo* pgai = packet;
        printf("sizeX = %u, sizeY = %u, blockID's = %s\n", pgai->sizeX, pgai->sizeY, pgai->blockIDs);
        for (i = 0; i < 169; i++) {
                printf("%d", pgai->blockIDs[i]);
        }
        printf("\n");
}

static void CallbackMovableObjects(void* packet, void* data) {
        struct PacketMovableObjects* pmo = packet;
        pmo = pmo;
}

static void CallbackServerMessage(void* packet, void* data) {
        struct PacketServerMessage* psm = packet;
        printf("message type = %u, message = %s\n", psm->messageType, psm->message);
}

static void CallbackServerPlayers(void* packet, void* data) {
        int i;
        struct PacketServerPlayers* psp = packet;
        printf("player count = %u\n\n", psp->playerCount);
        for (i = 0; i < psp->playerCount; i++) {
                printf("PlayerID = %u\n", psp->players[i].playerID);
                printf("Player name = %s\n", psp->players[i].playerName);
                printf("Player color = %u\n", psp->players[i].playerColor);
                printf("Player points = %u\n", psp->players[i].playerPoints);
                printf("Player lives = %u\n", psp->players[i].playerLives);
        }
        printf("\n");
       
}

int main()
{
        int my_socket = 0;      
        struct sockaddr_in remote_address;
        remote_address.sin_family = AF_INET;
        remote_address.sin_port = htons(PORT);
        unsigned char buffer[PACKET_MAX_BUFFER];
        struct PacketCallbacks pccbks;
        struct PacketClientId pcid;
        struct PacketClientInput pci;
        /* struct PacketClientMessage pcm; */
        unsigned int len;

        pcid.protoVersion = 0x00;
        strcpy(pcid.playerName, "Valters");
        pcid.playerColor = 'x';

        pci.movementX = 7;
        pci.movementY = 4;
        pci.action = 1;

        pccbks.callback[PACKET_TYPE_SERVER_IDENTIFY] = &CallbackServerId;
        pccbks.callback[PACKET_TYPE_SERVER_GAME_AREA] = &CallbacGameAreaInfo;
        pccbks.callback[PACKET_TYPE_MOVABLE_OBJECTS] = &CallbackMovableObjects;
        pccbks.callback[PACKET_TYPE_SERVER_MESSAGE] = &CallbackServerMessage;
        pccbks.callback[PACKET_TYPE_SERVER_PLAYER_INFO] = &CallbackServerPlayers;

        inet_pton(AF_INET, HOST, &remote_address.sin_addr);

        if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) /* new socket fd is returned */
        {
                printf("SOCKET ERROR\n");
                return -1;
        }

        if (connect(my_socket, (struct sockaddr*)&remote_address, sizeof(remote_address)) < 0)
        {
                printf("connection error");
                fprintf(stderr, "%d\n", errno);
                return -1;
        }
        else
        {
                /* Client ID */
                len = PacketEncode(buffer, PACKET_TYPE_CLIENT_IDENTIFY, &pcid);
                if (send(my_socket, buffer, len, 0) < 0) {
                        printf("error sending message");
                        return -1;
                }

                /* Accept or decline from serever */
                read(my_socket, &buffer[0], 13);
                if (buffer[0] == 0xff) {
                        PacketDecode(buffer, 13, &pccbks, NULL);
                }

                /* Client input */
                /* len = PacketEncode(buffer, PACKET_TYPE_CLIENT_INPUT, &pci);

                if (send(my_socket, buffer, len, 0) < 0) {
                        printf("error sending message");
                        return -1;
                } */

                while (1)
                {
                        if (read(my_socket, &buffer[0], 1) < 0) {
                                printf("No packet in buffer!\n");
                                fflush(NULL);
                        }
                        else if (buffer[0] == 0xff) {
                                if (read(my_socket, &buffer[1], 1) < 0) {
                                        printf("Couldn't read packet!\n");
                                        fflush(NULL);
                                }
                                if (buffer[1] == 0x00) {
                                        /* Type */
                                        if (read(my_socket, &buffer[2], 1) < 0) {
                                                printf("Couldn't read packet type!\n");
                                                fflush(NULL);
                                        }

                                        /* Length */
                                        if (read(my_socket, &buffer[3], 4) < 0) {
                                                printf("Couldn't read packet length!\n");
                                                fflush(NULL);
                                        }

                                        PACKET_BUFFER_PICK(buffer, 3, unsigned int, len, ntohl);

                                        /* Data */
                                        if (read(my_socket, &buffer[7], (len - 7)) < 0) {
                                                printf("Couldn't read packet data!\n");
                                                fflush(NULL);
                                        }
                                        /* Ckecksum */
                                        if (read(my_socket, &buffer[len], 1) < 0) {
                                                printf("Couldn't read packet checksum!\n");
                                                fflush(NULL);
                                        }

                                        len++;

                                        fflush(NULL);
                                        if (buffer[0] == 0xff) {
                                                PacketDecode(buffer, len, &pccbks, NULL);
                                        }

                                        printf("\n");
                                        fflush(NULL);
                                }
                        }

                }
        }
        return 0;
}