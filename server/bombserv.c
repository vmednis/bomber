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
#define PORT 3001
unsigned char buffer[PACKET_MAX_BUFFER];
int clientCount = 0;

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
        int len, type, err;

        pccbks.callback[PACKET_TYPE_CLIENT_IDENTIFY] = &CallbackClientId;
        pccbks.callback[PACKET_TYPE_CLIENT_INPUT] = &CallbackClientInput;
        pccbks.callback[PACKET_TYPE_CLIENT_MESSAGE] = &CallbackClientMessage;

        printf("Processing client id=%d, socket=%d.\n", id, socket);
        printf("CLIENT count %d\n", clientCount);
        /* while (1)
        {
                while (i < 2)
                {
                        printf("i1 = %d\n", i);
                        fflush(NULL);
                        len = recv(socket, buffer, sizeof(buffer), MSG_DONTWAIT);
                        if (len < 0) {
                                printf("ERROR Can't receive data.");
                                err = errno;
                                printf("ERROR = %d.\n", err);
                                printf("i2 = %d\n", i);
                                fflush(NULL);
                        }
                        printf("Length = %d. Type = %u\n", len, buffer[2]);
                        fflush(NULL);
                        PacketDecode(buffer, len, &pccbks, NULL);
                        printf("i3 = %d\n", i);
                        fflush(NULL);
                        if (len != -1) {
                                i++;
                        }
                }
        } */

        while (1) {
                read(socket, &buffer[0], 1);
                if (buffer[0] == 0xff) {
                        read(socket, &buffer[1], 1);
                        if (buffer[1] == 0x00) {
                                read(socket, &buffer[2], 1); /* Type */
                                type = buffer[2];
                                read(socket, &buffer[3], 1); /* Length */
                                len = buffer[2];
                                read(socket, &buffer[4], (len - 4)); /* Data */
                                read(socket, &buffer[4 + len], buffer[2]); /* Ckecksum */
                        }
                }
                printf("%u", buffer[2]);
                PacketDecode(buffer, len, &pccbks, NULL);
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