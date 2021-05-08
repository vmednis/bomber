#include <stdio.h>
#include <string.h>
#include <raylib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
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
void SetupCallbacks();
void DrawFrame(struct GameState* gameState, Texture2D* textureAtlas, float delta);
void SetupNetwork(struct NetworkState* netState);
void ProcessNetwork(struct GameState* gameState, struct NetworkState* netState);
void ProcessInput(struct GameState* gameState, struct NetworkState* netState);

void CallbackServerId(void * packet, void * passthrough);
void CallbackGameArea(void * packet, void * passthrough);

static struct PacketCallbacks packetCallbacks = {0};

int main() {
        float delta;
        struct NetworkState netState = {0};
        struct GameState gameState = {0};
        Texture2D textureAtlas[TEXTURE_ATLAS_SIZE] = {0};

        SelfTest();

        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bomber Client");
        LoadTextures(textureAtlas);
        SetupCallbacks();

        strcpy(netState.ip, "127.0.0.1");
        netState.port = 13337;
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

void SetupCallbacks() {
        packetCallbacks.callback[PACKET_TYPE_SERVER_IDENTIFY] = &CallbackServerId;
        packetCallbacks.callback[PACKET_TYPE_SERVER_GAME_AREA] = &CallbackGameArea;
}

void DrawFrame(struct GameState* gameState, Texture2D* textureAtlas, UNUSED float delta) {
        int x, y;
        unsigned char tile;

        ClearBackground(RAYWHITE);

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
}

void SetupNetwork(struct NetworkState* netState) {
        struct sockaddr_in addr;
        char yes = 1;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(netState->port);
        inet_pton(AF_INET, netState->ip, &addr.sin_addr);

        netState->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        setsockopt(netState->fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
        connect(netState->fd, (struct sockaddr *) &addr, sizeof(addr));
}

void ConsumePacket(unsigned char *buffer, unsigned int len, void *data) {
        unsigned int error = 0;

        if(len <= 0) return;
        if((error = PacketDecode(buffer, len, &packetCallbacks, data))) {
                printf("Error occured decoding packet %i", error);
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
                if (targetptr > 0 && ptr == targetptr) {
                        /*Ideal case we're good to read*/
                        ConsumePacket(buffer, ptr, gameState);
                        targetptr = 0;
                        ptr = 0;
                } else if (lastchar == 0xff && curchar == 0x00) {
                        /*Probably going to be malformed anyways, but worth a shot*/
                        ConsumePacket(buffer, ptr, gameState);
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
        }
}

void ProcessInput(struct GameState* gameState, struct NetworkState* netState) {
        struct PacketClientInput input = {0};
        unsigned char buffer[PACKET_MAX_BUFFER];
        int len = 0;

        struct PacketGameAreaInfo pgai = (struct PacketGameAreaInfo)pgai;

        if(gameState->timer > (1.0 / 5)) {
                if(IsKeyDown(KEY_RIGHT)) input.movementX += 127;
                if(IsKeyDown(KEY_LEFT))  input.movementX -= 127;
                if(IsKeyDown(KEY_UP))    input.movementY -= 127;
                if(IsKeyDown(KEY_DOWN))  input.movementY += 127;
                if(IsKeyDown(KEY_Z))     input.action = 1;

                len = PacketEncode(buffer, PACKET_TYPE_CLIENT_INPUT, &input);
                send(netState->fd, buffer, len, 0);

                gameState->timer = 0;
        }
}

void CallbackServerId(void * packet, void * passthrough) {
        struct PacketServerId* serverId = packet;
        struct GameState* gameState = passthrough;

        if (serverId->protoVersion == 0x0 && serverId->clientAccepted) {
                /* TODO: Switch some internal states or something*/
                gameState = gameState;
        }
}

void CallbackGameArea(void * packet, void * passthrough) {
        struct PacketGameAreaInfo* gameArea = packet;
        struct GameState* gameState = passthrough;

        gameState->worldX = gameArea->sizeX;
        gameState->worldY = gameArea->sizeY;
        memcpy(gameState->world, gameArea->blockIDs, gameArea->sizeY * gameArea->sizeX);
}