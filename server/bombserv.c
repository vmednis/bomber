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
#include <math.h>
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
        player->extra.player.toBeAccepted = 1;
}

static void CallbackClientInput(void* packet, void* data) {
        struct SourcedGameState* state = data;
        struct PacketClientInput* pcin = packet;
        struct GameObject* player;
        struct GameObject* bomb;
        float speed = 1.6;
        unsigned int i = 0;

        /* Apply player inputs */
        player = &state->gameState->objects[state->playerId];
        player->velx = pcin->movementX / 127.0 * speed;
        player->vely = pcin->movementY / 127.0 * speed;

        /* Try placing a bomb */
        if(pcin->action && player->extra.player.bombsRemaining) {
                while(i < MAX_OBJECTS) {
                        bomb = &state->gameState->objects[i];
                        if(!bomb->active) {
                                bomb->active = 1;
                                bomb->type = Bomb;
                                bomb->x = round(player->x);
                                bomb->y = round(player->y);
                                bomb->extra.bomb.owner = state->playerId;
                                bomb->extra.bomb.timeToDetonation = 3;
                                break;
                        }
                        i++;
                }
                player->extra.player.bombsRemaining--;
        }
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
                                                obj->x = gameState->spawnPoints[gameState->nextSpawnPoint].x;
                                                obj->y = gameState->spawnPoints[gameState->nextSpawnPoint].y;
                                                gameState->nextSpawnPoint++;
                                                gameState->nextSpawnPoint %= MAX_SPAWNPOINTS;
                                                obj->velx = 0;
                                                obj->vely = 0;
                                                obj->extra.player.fd = clientSocket;
                                                obj->extra.player.bombsRemaining = 1;
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
        struct PacketServerId packId = {0};
        struct PacketGameAreaInfo packWorld = {0};
        struct PacketMovableObjects packObjs = {0};
        struct PacketMovableObjectInfo* packObj;
        unsigned int fd;
        unsigned int len;
        int l2;
        unsigned int i;

        fd = gameState->objects[clientId].extra.player.fd;

        /* Send acceptance message */
        if(gameState->objects[clientId].extra.player.toBeAccepted) {
                gameState->objects[clientId].extra.player.toBeAccepted = 0;
                packId.protoVersion = 0x00;
                packId.clientAccepted = clientId;
                len = PacketEncode(buffer, PACKET_TYPE_SERVER_IDENTIFY, &packId);
                if(send(fd, buffer, len, 0) < 0) {
                        puts("Error sending game area");
                        /* What happens here? Do we remove the client or something? */
                }
        }

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

        return 0;
}

static void InitGameState(struct GameState* gameState) {
        /* Load default map */
        gameState->worldX = 13;
        gameState->worldY = 13;
        memcpy(gameState->world, blocks, gameState->worldX * gameState->worldY);

        gameState->spawnPoints[0] = (struct SpawnPoint) {1.0,  1.0};
        gameState->spawnPoints[1] = (struct SpawnPoint) {11.0, 11.0};
        gameState->spawnPoints[2] = (struct SpawnPoint) {1.0,  11.0};
        gameState->spawnPoints[3] = (struct SpawnPoint) {11.0, 1.0};
        gameState->nextSpawnPoint = 0;
}


static int IsWallCollidable(struct GameState* gameState, float x, float y) {
        unsigned char rx, ry;
        unsigned char wall;

        if(x < 0 || x + 1 > gameState->worldX) return 0;
        if(y < 0 || y + 1 > gameState->worldY) return 0;
        rx = y;
        ry = x;

        wall = gameState->world[rx * gameState->worldX + ry];

        if(wall == 1 || wall == 2) {
                /* Solid wall */
                return 1;
        } else {
                return 0;
        }
}

static float fclamp(float f, float from, float to) {
        if(f < from) {
                return from;
        } else if(f > to) {
                return to;
        }
        return f;
}

typedef struct Vec2 {
        float x;
        float y;
} Vec2;

static float MaxMovement(Vec2 opos, Vec2 osize, Vec2 omov, Vec2 wpos, Vec2 wsize) {
        float distx, disty, dist;

        /* Find x max movement */
        distx = 1;
        if(omov.x > 0) {
                distx = fabs(wpos.x - (opos.x + osize.x)) / omov.x;
        } else if(omov.x < 0) {
                distx = fabs((wpos.x + wsize.x) - opos.x) / (-1 * omov.x);
        }

        /* Find y max movement*/
        disty = 1;
        if(omov.y > 0) {
                disty = fabs(wpos.y - (opos.y + osize.y)) / omov.y;
        } else if(omov.y < 0) {
                disty = fabs((wpos.y + wsize.y) - opos.y) / (-1 * omov.y);
        }


        dist = distx < disty ? distx : disty;
        return fclamp(dist, 0, 1);
}

static float CheckCollision(struct GameState * state, Vec2 pos, Vec2 mov) {
        float distance = 1;
        float lx, rx, ty, by;
        float tmp;

        lx = floor(pos.x + mov.x + 0.01);
        rx = floor(pos.x + mov.x + 0.99);
        ty = floor(pos.y + mov.y + 0.01);
        by = floor(pos.y + mov.y + 0.99);

        tmp = 1;
        if(IsWallCollidable(state, lx, ty)) tmp = MaxMovement(pos, (Vec2) {1, 1}, mov, (Vec2) {lx, ty}, (Vec2) {1, 1});
        if(tmp < distance) distance = tmp;

        tmp = 1;
        if(IsWallCollidable(state, rx, ty)) tmp = MaxMovement(pos, (Vec2) {1, 1}, mov, (Vec2) {rx, ty}, (Vec2) {1, 1});
        if(tmp < distance) distance = tmp;

        tmp = 1;
        if(IsWallCollidable(state, rx, by)) tmp = MaxMovement(pos, (Vec2) {1, 1}, mov, (Vec2) {rx, by}, (Vec2) {1, 1});
        if(tmp < distance) distance = tmp;

        tmp = 1;
        if(IsWallCollidable(state, lx, by)) tmp = MaxMovement(pos, (Vec2) {1, 1}, mov, (Vec2) {lx, by}, (Vec2) {1, 1});
        if(tmp < distance) distance = tmp;

        return distance;
}

static void UpdateGameState(struct GameState* gameState, float delta) {
        struct GameObject* obj;
        unsigned char *wall;
        unsigned int i = 0;
        float dx, dy, dist;
        int bx, by, tc;

        /* Apply velocity*/
        while(i < MAX_OBJECTS) {
                obj = &gameState->objects[i];
                if(obj->active && obj->type == Player) {
                        /* While bombs can have velocity too currently ignore it to reduce complexity*/
                        dx = obj->velx * delta;
                        dy = obj->vely * delta;

                        dist = CheckCollision(gameState, (Vec2) {obj->x, obj->y}, (Vec2) {dx, dy});

                        obj->x += dx * dist;
                        obj->y += dy * dist;

                } else if(obj->active && obj->type == Bomb) {
                        /* Decrease bomb timer and explode if it's time */
                        obj->extra.bomb.timeToDetonation -= delta;
                        if(obj->extra.bomb.timeToDetonation < 0) {
                                /* BOOM */
                                obj->active = 0;
                                if(gameState->objects[obj->extra.bomb.owner].active) {
                                        /* Give a bomb back to the player */
                                        gameState->objects[obj->extra.bomb.owner].extra.player.bombsRemaining++;
                                }

                                /* Destruction */
                                by = floor(obj->y + 0.5);
                                bx = floor(obj->x + 0.5);
                                tc = bx + 2;
                                for(bx = bx; bx <= tc; bx++) {
                                        if(bx < 0 || bx > (int) gameState->worldX) continue;
                                        wall = &gameState->world[by * gameState->worldX + bx];
                                        if(*wall == 1) {
                                                break;
                                        }
                                        *wall = 0;
                                }
                                by = floor(obj->y + 0.5);
                                bx = floor(obj->x + 0.5);
                                tc = bx - 2;
                                for(bx = bx; bx >= tc; bx--) {
                                        if(bx < 0 || bx > (int) gameState->worldX) continue;
                                        wall = &gameState->world[by * gameState->worldX + bx];
                                        if(*wall == 1) {
                                                break;
                                        }
                                        *wall = 0;
                                }

                                by = floor(obj->y + 0.5);
                                bx = floor(obj->x + 0.5);
                                tc = by + 2;
                                for(by = by; by <= tc; by++) {
                                        if(by < 0 || by > (int) gameState->worldY) continue;
                                        wall = &gameState->world[by * gameState->worldX + bx];
                                        if(*wall == 1) {
                                                break;
                                        }
                                        *wall = 0;
                                }
                                by = floor(obj->y + 0.5);
                                bx = floor(obj->x + 0.5);
                                tc = by - 2;
                                for(by = by; by >= tc; by--) {
                                        if(by < 0 || by > (int) gameState->worldY) continue;
                                        wall = &gameState->world[by * gameState->worldX + bx];
                                        if(*wall == 1) {
                                                break;
                                        }
                                        *wall = 0;
                                }

                        }
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