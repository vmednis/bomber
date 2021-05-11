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
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "../shared/packet.h"
#include "bombserv.h"

#define HOST "127.0.0.1"
#define PORT 3001
unsigned char buffer[PACKET_MAX_BUFFER];
int clientFDs[MAX_CLIENTS];
int clientCount = 0;
int movableObjectCont = 0;
int startGame = 0;
int gameRunning = 0;
int mainSocket = 0;
char newClientName[32], newClienColor;
allClients* firstClient;
struct PacketGameAreaInfo pgai;

static void CallbackClientId(void* packet, void* data) {
        struct PacketClientId* pcid = packet;
        printf("protocol version = %u, player name = %s, player color = %c\n", pcid->protoVersion, pcid->playerName, pcid->playerColor);
        strcpy(newClientName, pcid->playerName);
        newClienColor = pcid->playerColor;
}

static void CallbackClientInput(void* packet, void* data) {
        struct PacketClientInput* pcin = packet;
        printf("movementX = %u, movementY = %u, action = %u\n", pcin->movementX, pcin->movementY, pcin->action);
}

static void CallbackClientMessage(void* packet, void* data) {
        struct PacketClientMessage* pcmsg = packet;
        pcmsg = pcmsg;
        data = data;
}

int startServer() {
        struct sockaddr_in serverAddress;

        if ((mainSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                printf("ERROR opening main server socket!\n");
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

        fcntl(mainSocket, F_SETFL, O_NONBLOCK);
        printf("Main socket is listening!\n");

        return 0;
}

int acceptClients() {
        int newClients = 1, clientSocket, len;
        struct sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);
        struct PacketServerId psid;
        allClients* currentClient = NULL;
        psid.protoVersion = 0x00;

        while (newClients) {
                clientSocket = accept(mainSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
                if (clientSocket == -1) {
                        newClients = 0;
                        if (errno != EWOULDBLOCK && errno != EAGAIN) {
                                printf("ERROR accepting client connection! ERRNO=%d\n", errno);
                                return -1;
                        }
                }
                else {
                        printf("Client succesfully connected!\n");
                        fcntl(clientSocket, F_SETFL, O_NONBLOCK);
                        if (clientCount > MAX_CLIENTS) {
                                psid.protoVersion = 0x00;
                                psid.clientAccepted = 0;
                                len = PacketEncode(buffer, PACKET_TYPE_SERVER_IDENTIFY, &psid);
                                if (send(clientSocket, buffer, len, 0) < 0) {
                                        printf("ERROR sending decline message to client!\n");
                                        return -1;
                                }
                        }
                        else {
                                if (clientCount == 0) {
                                        currentClient = (allClients*)malloc(sizeof(allClients));
                                        currentClient->client = (client*)malloc(sizeof(client));
                                        currentClient->client->clientID = clientCount;
                                        currentClient->client->fd = clientSocket;
                                        currentClient->next = NULL;
                                        firstClient = currentClient;

                                        clientFDs[clientCount] = clientSocket;
                                        clientCount++;
                                }
                                else {
                                        /* Iterate to last client */
                                        currentClient = firstClient;
                                        while (currentClient->next != NULL)
                                        {
                                                currentClient = currentClient->next;
                                        }

                                        currentClient->next = (allClients*)malloc(sizeof(allClients));
                                        currentClient = currentClient->next;
                                        currentClient->client = (client*)malloc(sizeof(client));
                                        currentClient->client->clientID = clientCount;
                                        currentClient->client->fd = clientSocket;
                                        currentClient->next = NULL;

                                        clientFDs[clientCount] = clientSocket;
                                        clientCount++;
                                }
                                psid.clientAccepted = 1;
                                len = PacketEncode(buffer, PACKET_TYPE_SERVER_IDENTIFY, &psid);
                                if (send(clientSocket, buffer, len, 0) < 0) {
                                        printf("ERROR sending accept message to client!\n");
                                        return -1;
                                }
                        }
                }
        }
        return 1;
}

int handleIncomingPackets() {
        struct PacketCallbacks pccbks;
        unsigned int len, i;
        unsigned char packetType;
        allClients* current = firstClient;

        pccbks.callback[PACKET_TYPE_CLIENT_IDENTIFY] = &CallbackClientId;
        pccbks.callback[PACKET_TYPE_CLIENT_INPUT] = &CallbackClientInput;
        pccbks.callback[PACKET_TYPE_CLIENT_MESSAGE] = &CallbackClientMessage;

        for (i = 0; i < clientCount; i++) {
                if (read(clientFDs[i], &buffer[0], 1) < 0) {
                        /* No data in client buffer */
                        continue;
                }
                else {
                        if (buffer[0] == 0xff) {
                                if (read(clientFDs[i], &buffer[1], 1) == -1) {
                                        printf("Couldn't read packet!\n");
                                        fflush(NULL);
                                }
                                if (buffer[1] == 0x00) {
                                        /* Type */
                                        if (read(clientFDs[i], &buffer[2], 1) < 0) {
                                                printf("Couldn't read packet type!\n");
                                                fflush(NULL);
                                        }
                                        packetType = buffer[2];

                                        /* Length */
                                        if (read(clientFDs[i], &buffer[3], 4) < 0) {
                                                printf("Couldn't read packet length!\n");
                                                fflush(NULL);
                                        }

                                        PACKET_BUFFER_PICK(buffer, 3, unsigned int, len, ntohl);

                                        /* Data */
                                        if (read(clientFDs[i], &buffer[7], (len - 7)) < 0) {
                                                printf("Couldn't read packet data!\n");
                                                fflush(NULL);
                                        }
                                        /* Ckecksum */
                                        if (read(clientFDs[i], &buffer[len], 1) < 0) {
                                                printf("Couldn't read packet checksum!\n");
                                                fflush(NULL);
                                        }

                                        len++;

                                        printf("Packet length = %d.\n", len);
                                        if (buffer[0] == 0xff) {
                                                PacketDecode(buffer, len, &pccbks, NULL);
                                        }
                                }
                        }

                }

                /* Do something depending on the received packet type */
                if (packetType == 0) {
                        while (current->next != NULL)
                        {
                                if (current->client->clientID == i) {
                                        strcpy(current->client->name, newClientName);
                                        current->client->color = newClienColor;
                                }
                                current = current->next;
                        }
                        if (current->client->clientID == i) {
                                strcpy(current->client->name, newClientName);
                                current->client->color = newClienColor;
                        }
                }
                else if (packetType == 1) {

                }
        }
        return 0;
}

/* Add current clients to the game */
int addPlayers() {
        allClients* current = firstClient;
        while (current->next != NULL) {
                current->client->inGame = 1;
                current = current->next;
        }
        current->client->inGame = 1;

        current = firstClient;
        while (current->next != NULL) {
                printf("Client %d in game = %d.\n", current->client->clientID, current->client->inGame);
                current = current->next;
        }
        printf("Client %d in game = %d.\n", current->client->clientID, current->client->inGame);
        return 1;
}

int updateClients() {
        int i, j, len;
        struct PacketGameAreaInfo pgai;
        struct PacketServerMessage psm;
        struct PacketMovableObjects pmo;
        struct PacketServerPlayers psp;
        struct PacketServerPlayerInfo players[clientCount];
        struct PacketMovableObjectInfo objects[MAX_MOVABLE_OBJECTS];
        allClients* current = firstClient;

        pgai.sizeX = 13;
        pgai.sizeY = 13;
        memcpy(pgai.blockIDs, blocks, sizeof(blocks));

        /* Send game arena to all players */
        for (i = 0; i < clientCount; i++) {
                len = PacketEncode(buffer, PACKET_TYPE_SERVER_GAME_AREA, &pgai);
                if (send(clientFDs[i], buffer, len, 0) < 0) {
                        printf("ERROR sending game arena");
                        return -1;
                }
                printf("Sent game arena to client %d.\n", i);
                /* Beginning of the round */
                if (startGame == 1) {
                        psm.messageType = 1;
                        strcpy(psm.message, "Game starting!");
                        len = PacketEncode(buffer, PACKET_TYPE_SERVER_MESSAGE, &psm);
                        if (send(clientFDs[i], buffer, len, 0) < 0) {
                                printf("ERROR sending message");
                                return -1;
                        }
                        printf("Sent message to client %d.\n", i);

                        /* Pack and send all player info */
                        psp.playerCount = clientCount;
                        for (j = 0; j < clientCount; j++) {
                                players[j].playerID = current->client->clientID;
                                strcpy(players[j].playerName, current->client->name);
                                players[j].playerColor = current->client->color;
                                players[j].playerPoints = 0;
                                players[j].playerLives = 3;
                                if (current->next != NULL) {
                                        current = current->next;
                                }
                                else {
                                        current = firstClient;
                                }
                        }
                        memcpy(psp.players, players, sizeof(players));

                        len = PacketEncode(buffer, PACKET_TYPE_SERVER_PLAYER_INFO, &psp);
                        if (send(clientFDs[i], buffer, len, 0) < 0) {
                                printf("ERROR sending player info");
                                return -1;
                        }
                        else {
                                printf("Sent player info to client %d!\n", i);
                        }

                        /* Pack and send all movable object info */
                        pmo.objectCount = clientCount;

                        for (j = 0; j < clientCount; j++) {
                                objects[j].objectType = 0;
                                objects[j].objectID = j;
                                if (j == 0) {
                                        objects[j].objectX = 2.5;
                                        objects[j].objectY = 2.5;
                                }
                                else if (j == 1) {
                                        objects[j].objectX = 12.5;
                                        objects[j].objectY = 12.5;
                                }
                                /* else if (j == 3) {
                                        objects[j].objectX = 12.5;
                                        objects[j].objectY = 2.5;
                                }
                                else if (j == 4) {
                                        objects[j].objectX = 12.5;
                                        objects[j].objectY = 12.5;
                                } */
                                objects[j].movement = 0;
                                objects[j].status = 3;
                        }
                        memcpy(pmo.movableObjects, objects, sizeof(objects));

                        len = PacketEncode(buffer, PACKET_TYPE_MOVABLE_OBJECTS, &pmo);
                        if (send(clientFDs[i], buffer, len, 0) < 0) {
                                printf("ERROR sending player info");
                                return -1;
                        }
                        else {
                                printf("Sent movable object info to client %d!\n", i);
                        }
                }


                /* if (current->client->inGame == 1) {

                } */
        }
        startGame = 0;
        return 0;
}

int gameloop() {
        while (1)
        {
                acceptClients();
                handleIncomingPackets();

                /* Start game */
                if (gameRunning == 0 && clientCount >= 2) {
                        printf("Client count = %d.\n", clientCount);
                        addPlayers();
                        startGame = 1;
                        gameRunning = 1;
                        printf("Starting game!\n");
                        updateClients();
                }
                sleep(1);
        }
        return 0;
}

int main()
{
        if (startServer() < 0) {
                printf("ERROR starting server!\n");
                fflush(NULL);
        }
        else {
                gameloop();
        }
        return 0;
}