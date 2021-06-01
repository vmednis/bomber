#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "../shared/packet.h"
#include "../client/netcallbacks.h"
#include "../client/tests.h"
#include "../client/network.h"
#include "../client/gamestate.h"
#include "../client/textures.h"
#include "../client/hashmap.h"

#define UNUSED __attribute__((unused))
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

void LoadTextures(Texture2D* atlas);
void SetupGameState(struct GameState * gameState);
void SetupCallbacks();
void UpdateTimers(struct GameState * gameState, float delta);
void CalcInterpolation(struct GameState * gameState);
void DrawFrame(struct GameState* gameState, Texture2D* textureAtlas, float delta);
void SetupNetwork(struct NetworkState* netState);
void ProcessNetwork(struct GameState* gameState, struct NetworkState* netState);
void ProcessInput(struct GameState* gameState, struct NetworkState* netState);
void UpdateCameraBorder(struct GameState* state, int width, int height);

static struct PacketCallbacks packetCallbacks = {0};

int main(int argc, char **argv) {
        float delta;
        struct NetworkState netState = {0};
        struct GameState gameState = {0};
        Texture2D textureAtlas[TEXTURE_ATLAS_SIZE] = {0};

        SelfTest();

        if(argc < 4) {
                puts("Missing arguments!");
                puts("./bombcli ip port name");
                return 1;
        }

        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bomber Client");
        LoadTextures(textureAtlas);
        SetupGameState(&gameState);
        SetupCallbacks();

        strcpy(netState.ip, argv[1]);
        netState.port = atoi(argv[2]);
        strcpy(netState.reqName, argv[3]);
        SetupNetwork(&netState);

        while(!WindowShouldClose()) {
                delta = GetFrameTime();

                ProcessInput(&gameState, &netState);
                UpdateTimers(&gameState, delta);
                ProcessNetwork(&gameState, &netState);
                CalcInterpolation(&gameState);
                UpdateCameraBorder(&gameState, SCREEN_WIDTH, SCREEN_HEIGHT);

                BeginDrawing();
                        DrawFrame(&gameState, textureAtlas, delta);
                EndDrawing();
        }

        CloseWindow();
        return 0;
}

void LoadTextures(Texture2D* atlas) {
        atlas[TEXTURE_WALL_SOLID] = LoadTexture("assets/wall_solid.png");
        atlas[TEXTURE_WALL_BREAKABLE] = LoadTexture("assets/wall_breakable.png");
        atlas[TEXTURE_PLAYER_FRONT] = LoadTexture("assets/player_front.png");
        atlas[TEXTURE_PLAYER_LEFT] = LoadTexture("assets/player_left.png");
        atlas[TEXTURE_PLAYER_RIGHT] = LoadTexture("assets/player_right.png");
        atlas[TEXTURE_PLAYER_BACK] = LoadTexture("assets/player_back.png");
        atlas[TEXTURE_BOMB] = LoadTexture("assets/bomb.png");
}

void SetupGameState(struct GameState * gameState) {
        gameState->objects = HashmapNew();
        gameState->players = HashmapNew();
        gameState->camera.zoom = 1.0f;
        gameState->objUpdateLen = 0.0f;
}

void SetupCallbacks() {
        packetCallbacks.callback[PACKET_TYPE_SERVER_IDENTIFY] = &CallbackServerId;
        packetCallbacks.callback[PACKET_TYPE_SERVER_GAME_AREA] = &CallbackGameArea;
        packetCallbacks.callback[PACKET_TYPE_MOVABLE_OBJECTS] = &CallbackMovableObj;
        packetCallbacks.callback[PACKET_TYPE_SERVER_MESSAGE] = &CallbackMessage;
        packetCallbacks.callback[PACKET_TYPE_SERVER_PLAYER_INFO] = &CallbackServerPlayers;
        packetCallbacks.callback[PACKET_TYPE_SERVER_PING] = &CallbackPing;
}

Color CalculatePlayerColor(unsigned int color) {
        float H = color / 255.0;
        float S = 0.80;
        float V = 0.85;
        float R, G, B;
        float C, X, m;

        /* Formula for HSV to RGB conversion shamelessly stolen from:
        https://www.rapidtables.com/convert/color/hsv-to-rgb.html*/
        C = V * S;
        X = C * (1.0 - (fmod((H / (1.0 / 6.0)), 2.0) - 1.0));
        m = V - C;

        if (H < 1.0 / 6.0) {
                R = C;
                G = X;
                B = 0;
        } else if (H < 2.0 / 6.0) {
                R = X;
                G = C;
                B = 0;
        } else if (H < 3.0 / 6.0) {
                R = 0;
                G = C;
                B = X;
        } else if (H < 4.0 / 6.0) {
                R = 0;
                G = X;
                B = C;
        } else if (H < 5.0 / 6.0) {
                R = X;
                G = 0;
                B = C;
        } else {
                R = C;
                G = 0;
                B = X;
        }

        R = (R + m) * 255.0;
        G = (G + m) * 255.0;
        B = (B + m) * 255.0;
        return (Color) {(unsigned char) R, (unsigned char) G, (unsigned char) B, 255};
}

void UpdateTimers(struct GameState * gameState, float delta) {
        gameState->timer += delta;
        gameState->timerObjUpdate += delta;
}

void DrawFrame(struct GameState* gameState, Texture2D* textureAtlas, UNUSED float delta) {
        int x, y;
        unsigned char tile;
        struct HashmapIterator* iter;
        struct GameObject* curobj;

        ClearBackground((Color) {0x5b, 0x5d, 0x60, 0xff});

        /*Enter world space*/
        BeginMode2D(gameState->camera);
        /* Draw world */
        for(y = 0; y < gameState->worldY; y++) {
                for(x = 0; x < gameState->worldX; x++) {
                        tile = gameState->world[y * gameState->worldX + x];
                        if(tile != 0) {
                                DrawTexture(textureAtlas[tile], x * 64, y * 64, WHITE);
                        }
                }
        }

        /* Draw objects */
        iter = HashmapIterator(gameState->objects);
        while((curobj = HashmapNext(iter)) != NULL) {
                switch(curobj->type) {
                case Player:
                        DrawTexture(textureAtlas[TEXTURE_PLAYER_FRONT], curobj->ix * 64.0, curobj->iy * 64.0, CalculatePlayerColor(curobj->tint));
                        break;
                case Bomb:
                        DrawTexture(textureAtlas[TEXTURE_BOMB], curobj->ix * 64.0, curobj->iy * 64.0, WHITE);
                        break;
                }
        }

        EndMode2D();
}

void SetupNetwork(struct NetworkState* netState) {
        struct sockaddr_in addr;
        char yes = 1;
        struct PacketClientId pci = {0};
        unsigned char buffer[PACKET_MAX_BUFFER];
        unsigned int len;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(netState->port);
        inet_pton(AF_INET, netState->ip, &addr.sin_addr);

        /* Connect to the server */
        netState->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        setsockopt(netState->fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
        connect(netState->fd, (struct sockaddr *) &addr, sizeof(addr));

        /* Introduce yourself ot the server */
        pci.protoVersion = 0x00;
        strcpy(pci.playerName, netState->reqName);
        pci.playerColor = 'x';

        len = PacketEncode(buffer, PACKET_TYPE_CLIENT_IDENTIFY, &pci);
        send(netState->fd, buffer, len, 0);
}

void ConsumePacket(unsigned char *buffer, unsigned int len, void *data) {
        if(len <= 0) return;
        switch (PacketDecode(buffer, len, &packetCallbacks, data))
        {
        case PACKET_ERR_DECODE_START:
                puts("Packet decode error: Malformed start");
                break;
        case PACKET_ERR_DECODE_UNIMPLEMENTED:
                puts("Packet decode error: Unimplemented packet type");
                break;
        case PACKET_ERR_DECODE_CHECKSUM:
                puts("Packet decode error: Checksum incorrect");
                break;
        case PACKET_ERR_DECODE_OTHER:
                puts("Packet decode error: Unknown Error");
                break;
        }
}

void ProcessNetwork(struct GameState* gameState, struct NetworkState* netState) {
        static unsigned char buffer[PACKET_MAX_BUFFER];
        static unsigned char curchar = 0;
        static unsigned char lastchar = 0;
        static unsigned int ptr = 0;
        static unsigned int targetptr = 0;
        static int len = 0;

        while((len = read(netState->fd, &curchar, 1)) > 0) {
                if (ptr != 0 && lastchar == 0xff && curchar == 0x00) {
                        /*Probably going to be malformed anyways, but worth a shot*/
                        ConsumePacket(buffer, ptr - 1, gameState);
                        buffer[0] = lastchar;
                        targetptr = 0;
                        ptr = 1;
                }

                if (ptr == 7) {
                        targetptr = ntohl(*((int *)(&buffer[ptr - 4])));
                }
                buffer[ptr] = curchar;
                lastchar = curchar;
                ptr++;

                if (targetptr > 0 && ptr == (targetptr + 1)) {
                        /*Ideal case we're good to read*/
                        ConsumePacket(buffer, ptr, gameState);
                        targetptr = 0;
                        ptr = 0;
                }
        }
}

void CalcInterpolation(struct GameState * gameState) {
        struct HashmapIterator* iter;
        struct GameObject *curobj;
        float prog;

        prog = gameState->timerObjUpdate / gameState->objUpdateLen;
        prog = prog > 1.0 ? 1.0 : prog;

        iter = HashmapIterator(gameState->objects);
        while((curobj = HashmapNext(iter)) != NULL) {
                curobj->ix = curobj->prevx + (curobj->x - curobj->prevx) * prog;
                curobj->iy = curobj->prevy + (curobj->y - curobj->prevy) * prog;
        }
}

void ProcessInput(struct GameState* gameState, struct NetworkState* netState) {
        struct PacketClientInput input = {0};
        unsigned char buffer[PACKET_MAX_BUFFER];
        int len = 0;

        if(gameState->timer > (1.0 / 20)) {
                if(IsKeyDown(KEY_RIGHT)) input.movementX += 127;
                if(IsKeyDown(KEY_LEFT))  input.movementX -= 127;
                if(IsKeyDown(KEY_UP))    input.movementY -= 127;
                if(IsKeyDown(KEY_DOWN))  input.movementY += 127;
                if(IsKeyDown(KEY_Z))     input.action = 1;

                len = PacketEncode(buffer, PACKET_TYPE_CLIENT_INPUT, &input);
                send(netState->fd, buffer, len, 0);

                gameState->timer = 0;
        }

        if(gameState->pingrequested) {
                gameState->pingrequested = 0;
                len = PacketEncode(buffer, PACKET_TYPE_CLIENT_PING_ANSWER, NULL);
                send(netState->fd, buffer, len, 0);
        }
}

void UpdateCameraBorder(struct GameState* state, int width, int height)
{
        /*Based on the camera example code https://www.raylib.com/examples.html */
        Vector2 border = { 0.4f, 0.4f };
        struct GameObject* player;
        struct Camera2D* camera;

        camera = &state->camera;
        if(!(player = HashmapGet(state->objects, state->playerId))) {
                /* No active player on the grund*/
                return;
        }

        Vector2 borderWorldMin = GetScreenToWorld2D((Vector2){ (1 - border.x)*0.5f*width, (1 - border.y)*0.5f*height }, *camera);
        Vector2 borderWorldMax = GetScreenToWorld2D((Vector2){ (1 + border.x)*0.5f*width, (1 + border.y)*0.5f*height }, *camera);
        camera->offset = (Vector2){ (1 - border.x)*0.5f * width, (1 - border.y)*0.5f*height };

        if (player->x * 64 < borderWorldMin.x) camera->target.x = player->x * 64;
        if (player->y * 64 < borderWorldMin.y) camera->target.y = player->y * 64;
        if (player->x * 64 > borderWorldMax.x) camera->target.x = borderWorldMin.x + (player->x * 64 - borderWorldMax.x);
        if (player->y * 64 > borderWorldMax.y) camera->target.y = borderWorldMin.y + (player->y * 64 - borderWorldMax.y);
}