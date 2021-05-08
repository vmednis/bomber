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
#include <sys/wait.h>
#include "../shared/packet.h"

#define HOST "127.0.0.1"
#define PORT 3000
unsigned char buffer[PACKET_MAX_BUFFER];
int clientCount = 0;

/* tmp */
#define PACKET_BUFFER_PICK(buffer, offset, type, value, converter) do { \
        value = converter(*(type *)((void *) (buffer + offset))); \
} while(0);

static void CallbackClientId(void* packet, void* data) {
        struct PacketClientId* pcid = packet;
        printf("protocol version = %u, player name = %s, player color = %c\n", pcid->protoVersion, pcid->playerName, pcid->playerColor);
        fflush(NULL);
}

static void CallbackClientInput(void* packet, void* data) {
        struct PacketClientInput* pcin = packet;
        printf("movementX = %u, movementY = %u, action = %u\n", pcin->movementX, pcin->movementY, pcin->action);
        fflush(NULL);
}

static void CallbackClientMessage(void* packet, void* data) {
        struct PacketClientMessage* pcmsg = packet;
        pcmsg = pcmsg;
        data = data;
}

int processClient(int id, int socket)
{
        struct PacketCallbacks pccbks;
        unsigned int packetLength, packetType, i = 0;

        pccbks.callback[PACKET_TYPE_CLIENT_IDENTIFY] = &CallbackClientId;
        pccbks.callback[PACKET_TYPE_CLIENT_INPUT] = &CallbackClientInput;
        pccbks.callback[PACKET_TYPE_CLIENT_MESSAGE] = &CallbackClientMessage;

        printf("Processing client id = %d, socket = %d.\n", id, socket);
        printf("CLIENT count = %d.\n", clientCount);

        while (1) {
                if (read(socket, &buffer[0], 1) < 0) {
                        printf("No packet in buffer!\n");
                        fflush(NULL);
                }
                else if (buffer[0] == 0xff) {
                        if (read(socket, &buffer[1], 1) < 0) {
                                printf("Couldn't read packet!\n");
                                fflush(NULL);
                        }
                        if (buffer[1] == 0x00) {
                                /* Type */
                                if (read(socket, &buffer[2], 1) < 0) {
                                        printf("Couldn't read packet type!\n");
                                        fflush(NULL);
                                }
                                packetType = buffer[2];

                                /* Length */
                                if (read(socket, &buffer[3], 4) < 0) {
                                        printf("Couldn't read packet length!\n");
                                        fflush(NULL);
                                }

                                PACKET_BUFFER_PICK(buffer, 3, unsigned int, packetLength, ntohl);

                                /* Data */
                                if (read(socket, &buffer[7], (packetLength - 7)) < 0) {
                                        printf("Couldn't read packet data!\n");
                                        fflush(NULL);
                                }
                                /* Ckecksum */
                                if (read(socket, &buffer[packetLength], 1) < 0) {
                                        printf("Couldn't read packet checksum!\n");
                                        fflush(NULL);
                                }

                                packetLength++;

                                /* New client */
                                if (packetType == 0) {

                                }

                                printf("Packet length = %d.\n", packetLength);
                                fflush(NULL);
                                if (buffer[0] == 0xff) {
                                        PacketDecode(buffer, packetLength, &pccbks, NULL);
                                }

                                i = 0;
                                while (i < packetLength) {
                                        buffer[i] = 0x00;
                                        printf("%u", buffer[i]);
                                        fflush(NULL);
                                        i++;

                                }
                                printf("\n");
                                fflush(NULL);
                        }
                }
        }
        return 0;
}

int startNetwork()
{
        int mainSocket;
        int clientSocket;
        struct sockaddr_in serverAddress;
        struct sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);

        if ((mainSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                printf("ERROR opening main server socket\n");
                exit(1);
        };
        printf("Main socket created!\n");

        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(PORT);

        if (bind(mainSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
        {
                printf("ERROR binding the main server socket!\n");
                printf("ERROR %d\n", errno);
                exit(1);
        }
        printf("Main socket binded!\n");

        if (listen(mainSocket, MAX_CLIENTS) < 0)
        {
                printf("ERROR listening to socket!\n");
                exit(1);
        }
        printf("Main socket is listening!\n");

        while (1)
        {
                int newClientID = 0, cpid = 0;

                clientSocket = accept(mainSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
                if (clientSocket < 0)
                {
                        printf("ERROR accepting client connection! ERRNO=%d\n", errno);
                        continue;
                }
                newClientID = clientCount;
                clientCount += 1;
                cpid = fork();

                if (cpid == 0) /* Child process */
                {
                        close(mainSocket);
                        cpid = fork();
                        if (cpid == 0) /* Grandchild process*/
                        {
                                processClient(newClientID, clientSocket);
                                exit(0);
                        }
                        else
                        {
                                wait(NULL);
                                printf("Successfully orphaned client %d\n", newClientID);
                                exit(0);
                        }
                }
                else /* Parent process */
                {
                        close(clientSocket);
                }
        }
        return 0;
}

int main()
{
        startNetwork();
        return 0;
}