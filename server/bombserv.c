#define _DEFAULT_SOURCE
#define UNUSED __attribute__((unused))
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
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
int mainSocket = 0;

static void CallbackClientId(void* packet, void* data) {
        struct SourcedGameState* state = data;
        struct PacketClientId* pcid = packet;
        struct GameObject* player;

        /*Set personalization options on player*/
        player = &state->gameState->objects[state->playerId];
        strcpy(player->extra.player.name, pcid->playerName);
        player->extra.player.color = pcid->playerColor;
}

static void CallbackClientInput(void* packet, void* data) {
        struct SourcedGameState* state = data;
        struct PacketClientInput* pcin = packet;
        struct GameObject* player;
        float speed = 0.8;

        /* Apply player inputs */
        player = &state->gameState->objects[state->playerId];
        player->velx = pcin->movementX / 127.0 * speed;
        player->vely = pcin->movementY / 127.0 * speed;
}

static void CallbackClientMessage(void* packet, void* data) {
        struct PacketClientMessage* pcmsg = packet;
        pcmsg = pcmsg;
        data = data;
}

static void CallbackClientPing(UNUSED void* packet, void* data) {
        data = data;
}

int StartServer() {
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

        fcntl(mainSocket, F_SETFL, fcntl(mainSocket, F_GETFL) | O_NONBLOCK);
        printf("Main socket is listening!\n");

        return 0;
}

int AcceptClients(struct GameState* gameState) {
        unsigned char buffer[PACKET_MAX_BUFFER];
        int newClients = 1, clientSocket, len;
        struct sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);
        struct PacketServerId psid;
        struct GameObject* obj;
        unsigned int clientCount = 0;
        unsigned int i = 0;
        char yes = 1;

        while(i < MAX_OBJECTS) {
                obj = &gameState->objects[i];
                if(obj->active && obj->type == Player) {
                        clientCount++;
                }
                i++;
        }

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
                        fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL) | O_NONBLOCK);
                        if (setsockopt(clientSocket, SOL_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1) {
                                /* printf("ERROR setting option TCP_NODELAY to client socket = %d.\n", errno); */
                        }

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
                                /* Find a free spot in objects table, that will be client id*/
                                /* DANGER: Client Id can't be 0 so we start at 1*/
                                i = 1;
                                while(i < MAX_OBJECTS) {
                                        if(!gameState->objects[i].active) {
                                                obj = &gameState->objects[i];
                                                obj->active = 1;
                                                obj->type = Player;
                                                obj->x = 1;
                                                obj->y = 1;
                                                obj->velx = 0;
                                                obj->vely = 0;
                                                obj->extra.player.fd = clientSocket;
                                                return 1;
                                        }
                                        i++;
                                }
                        }
                }
        }
        return 1;
}

/* Handles Client Packets, returns 1 if it read any packet, 0 if there currently are no more packets. */
int HandleClientPackets(int clientId, struct GameState* gameState, struct PacketCallbacks *pckcbks) {
        struct SourcedGameState sourcedGameState = {0};
        unsigned char buffer[PACKET_MAX_BUFFER];
        unsigned int fd = gameState->objects[clientId].extra.player.fd;
        unsigned int len;

        if (read(fd, &buffer[0], 1) < 0) {
                return 0;
        }

        if (buffer[0] == 0xff) {
                if (read(fd, &buffer[1], 1) == -1) {
                        printf("Couldn't read packet!\n");
                        fflush(NULL);
                }

                if (buffer[1] == 0x00) {
                        /* Type */
                        if (read(fd, &buffer[2], 1) < 0) {
                                printf("Couldn't read packet type!\n");
                                fflush(NULL);
                        }

                        /* Length */
                        if (read(fd, &buffer[3], 4) < 0) {
                                printf("Couldn't read packet length!\n");
                                fflush(NULL);
                        }

                        PACKET_BUFFER_PICK(buffer, 3, unsigned int, len, ntohl);

                        /* Data */
                        if (read(fd, &buffer[7], (len - 7)) < 0) {
                                printf("Couldn't read packet data!\n");
                                fflush(NULL);
                        }
                        /* Ckecksum */
                        if (read(fd, &buffer[len], 1) < 0) {
                                printf("Couldn't read packet checksum!\n");
                                fflush(NULL);
                        }

                        len++;

                        if (buffer[0] == 0xff) {
                                sourcedGameState.playerId = clientId;
                                sourcedGameState.gameState = gameState;
                                PacketDecode(buffer, len, pckcbks, &sourcedGameState);
                        }
                }
        }

        return 1;
}

int HandleIncomingPackets(struct GameState* gameState) {
        struct PacketCallbacks cbks;
        struct GameObject* obj;
        unsigned int i = 0;

        cbks.callback[PACKET_TYPE_CLIENT_IDENTIFY] = &CallbackClientId;
        cbks.callback[PACKET_TYPE_CLIENT_INPUT] = &CallbackClientInput;
        cbks.callback[PACKET_TYPE_CLIENT_MESSAGE] = &CallbackClientMessage;
        cbks.callback[PACKET_TYPE_CLIENT_PING_ANSWER] = &CallbackClientPing;

        while(i < MAX_OBJECTS) {
                obj = &gameState->objects[i];
                if(obj->active && obj->type == Player) {
                        /* There can be multiple packets per update from one client*/
                        while(HandleClientPackets(i, gameState, &cbks));
                }
                i++;
        }
        return 0;
}

int UpdateClient(int clientId, struct GameState* gameState) {
        unsigned char buffer[PACKET_MAX_BUFFER];
        struct GameObject* obj;
        struct PacketGameAreaInfo packWorld = {0};
        struct PacketMovableObjects packObjs = {0};
        struct PacketMovableObjectInfo* packObj;
        unsigned int fd;
        unsigned int len;
        int l2;
        unsigned int i;

        fd = gameState->objects[clientId].extra.player.fd;

        /* Sends world */
        packWorld.sizeX = gameState->worldX;
        packWorld.sizeY = gameState->worldY;
        memcpy(packWorld.blockIDs, gameState->world, packWorld.sizeX * packWorld.sizeY);
        len = PacketEncode(buffer, PACKET_TYPE_SERVER_GAME_AREA, &packWorld);
        if(send(fd, buffer, len, 0) < 0) {
                puts("Error sending game area");
                /* What happens here? Do we remove the client or something? */
        }

        /* Sends game objects */
        i = 0;
        while (i < MAX_OBJECTS) {
                obj = &gameState->objects[i];
                if(obj->active) {
                        packObj = &packObjs.movableObjects[packObjs.objectCount];
                        packObj->objectType = obj->type;
                        packObj->objectID = i;
                        packObj->objectX = obj->x;
                        packObj->objectY = obj->y;
                        packObjs.objectCount++;
                }
                i++;
        }
        len = PacketEncode(buffer, PACKET_TYPE_MOVABLE_OBJECTS, &packObjs);
        if((l2 = send(fd, buffer, len, 0)) < 0) {
                puts("Error sending movable objects");
                /* What happens here? Do we remove the client or something? */
        }

        return 0;
}

int UpdateClients(struct GameState* gameState) {
        struct GameObject* obj;
        unsigned int i = 0;

        while(i < MAX_OBJECTS) {
                obj = &gameState->objects[i];
                if(obj->active && obj->type == Player) {
                        UpdateClient(i, gameState);
                }
                i++;
        }
        /*
        unsigned char buffer[PACKET_MAX_BUFFER];
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

        for (i = 0; i < clientCount; i++) {
                len = PacketEncode(buffer, PACKET_TYPE_SERVER_GAME_AREA, &pgai);
                if (send(clientFDs[i], buffer, len, 0) < 0) {
                        printf("ERROR sending game arena");
                        return -1;
                }
                printf("Sent game arena to client %d.\n", i);
                if (startGame == 1) {
                        psm.messageType = 1;
                        strcpy(psm.message, "Game starting!");
                        len = PacketEncode(buffer, PACKET_TYPE_SERVER_MESSAGE, &psm);
                        if (send(clientFDs[i], buffer, len, 0) < 0) {
                                printf("ERROR sending message");
                                return -1;
                        }
                        printf("Sent message to client %d.\n", i);

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

=                        pmo.objectCount = clientCount;

                        for (j = 0; j < clientCount; j++) {
                                objects[j].objectType = 0;
                                objects[j].objectID = j;
                                if (j == 0) {
                                        objects[j].objectX = 1.0;
                                        objects[j].objectY = 1.0;
                                }
                                else if (j == 1) {
                                        objects[j].objectX = 11.0;
                                        objects[j].objectY = 11.0;
                                }
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

        }
        startGame = 0;
        */
        return 0;
}

static void InitGameState(struct GameState* gameState) {
        /* Load default map */
        gameState->worldX = 13;
        gameState->worldY = 13;
        memcpy(gameState->world, blocks, gameState->worldX * gameState->worldY);
}

static void UpdateGameState(struct GameState* gameState, float delta) {
        struct GameObject* obj;
        unsigned int i = 0;

        /* Apply velocity*/
        while(i < MAX_OBJECTS) {
                obj = &gameState->objects[i];
                if(obj->active && obj->type == Player) {
                        /* While bombs can have velocity too currently ignore it to reduce complexity*/
                        obj->x += obj->velx * delta;
                        obj->y += obj->vely * delta;

                        /*Todo check colision with walls, easiest way if will be intersecting wall after this don't allow the movement*/
                        /*More advanced could do some sliding techniques, but yeah nah*/
                        /*Also better to keep players bounding box a bit smaller than 1x1 to make turning corners easier*/
                }
                i++;
        }

}

int GameLoop() {
        struct GameState gameState = {0};
        InitGameState(&gameState);

        while (1)
        {
                AcceptClients(&gameState);
                HandleIncomingPackets(&gameState);
                UpdateGameState(&gameState, 0.1);
                UpdateClients(&gameState);

                /*Should be replaced with proper deltas asap*/
                usleep(1000 * 100);
        }
        return 0;
}

int main()
{
        if (StartServer() < 0) {
                printf("ERROR starting server!\n");
                fflush(NULL);
        }
        else {
                GameLoop();
        }
        return 0;
}