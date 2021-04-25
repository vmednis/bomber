#include <stdio.h>
#include <string.h>
#include <raylib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../shared/packet.h"
#include "../client/tests.h"
#include "../client/network.h"
#include "../client/gamestate.h"
#include "../client/texture_atlas.h"

#define UNUSED __attribute__((unused))
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

void LoadTextures(Texture2D* atlas);
void DrawFrame(struct GameState* gameState, Texture2D* textureAtlas, float delta);
void SetupNetwork(struct NetworkState* netState);
void ProcessNetwork(struct GameState* gameState, struct NetworkState* netState);
void ProcessInput(struct GameState* gameState, struct NetworkState* netState);

int main() {
        float delta;
        struct NetworkState netState = {0};
        struct GameState gameState = {0};
        Texture2D textureAtlas[TEXTURE_ATLAS_SIZE] = {0};

        SelfTest();

        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bomber Client");
        LoadTextures(textureAtlas);

        strcpy(netState.ip, "127.0.0.1");
        netState.port = 1337;
        SetupNetwork(&netState);

        while(!WindowShouldClose()) {
                delta = GetFrameTime();

                ProcessInput(&gameState, &netState);
                ProcessNetwork(&gameState, &netState);
                gameState.timer += delta;

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
}

void DrawFrame(struct GameState* gameState, Texture2D* textureAtlas, UNUSED float delta) {
        int i, j;
        unsigned char tile;

        ClearBackground(RAYWHITE);

        /* Draw world */
        for(i = 0; i < gameState->worldY; i++) {
                for(j = 0; j < gameState->worldX; j++) {
                        tile = gameState->world[i][j];
                        if(tile != 0) {
                                DrawTexture(textureAtlas[tile], j * 64, i * 64, WHITE);
                        }
                }
        }

        /* Draw objects */
}

void SetupNetwork(struct NetworkState* netState) {
        struct sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(netState->port);
        inet_pton(AF_INET, netState->ip, &addr.sin_addr);

        netState->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        connect(netState->fd, (struct sockaddr *) &addr, sizeof(addr));
}

void ProcessNetwork(struct GameState* gameState, struct NetworkState* netState) {
        struct PacketCallbacks pckcbks = {0};
        unsigned char buffer[PACKET_MAX_BUFFER];
        int len = 0;

        len = read(netState->fd, buffer, PACKET_MAX_BUFFER);
        if(len > 0) {
                PacketDecode(buffer, len, &pckcbks, gameState);
        }
}

void ProcessInput(struct GameState* gameState, struct NetworkState* netState) {
        struct PacketClientInput input = {0};
        unsigned char buffer[PACKET_MAX_BUFFER];
        int len = 0;

        if(gameState->timer > 1) {
                if(IsKeyDown(KEY_RIGHT)) input.movementX += 127;
                if(IsKeyDown(KEY_LEFT))  input.movementX -= 127;
                if(IsKeyDown(KEY_UP))    input.movementY -= 127;
                if(IsKeyDown(KEY_DOWN))  input.movementY += 127;
                if(IsKeyDown(KEY_Z))     input.action = 1;

                len = PacketEncode(buffer, PACKET_TYPE_CLIENT_INPUT, &input);
                write(netState->fd, buffer, len);

                gameState->timer = 0;
        }
}