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
void CallbackMovableObj(void * packet, void * passthrough);
void CallbackMessage(void * packet, void * passthrough);
void CallbackPlayerInfo(void * packet, void * passthrough);
void CallbackPing(void * packet, void * passthrough);

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
        netState.port = 3001;
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
        packetCallbacks.callback[PACKET_TYPE_MOVABLE_OBJECTS] = &CallbackMovableObj;
        packetCallbacks.callback[PACKET_TYPE_SERVER_MESSAGE] = &CallbackMessage;
        packetCallbacks.callback[PACKET_TYPE_SERVER_PLAYER_INFO] = &CallbackPlayerInfo;
        packetCallbacks.callback[PACKET_TYPE_SERVER_PING] = &CallbackPing;
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
        strcpy(pci.playerName, "Client");
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

void CallbackMovableObj(void * packet, void * passthrough) {
        struct PacketMovableObjects* movable = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled MovableObj: count=%i", movable->objectCount);
}

void CallbackMessage(void * packet, void * passthrough) {
        struct PacketServerMessage* msg = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled ServerMsg: type=%i, data=%s", msg->messageType, msg->message);
}

void CallbackPlayerInfo(void * packet, void * passthrough) {
        struct PacketServerPlayerInfo* info = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled player info: id=%i, name=%s", info->playerID, info->playerName);
}

void CallbackPing(void * packet, void * passthrough) {
        struct PacketServerPing* ping = packet;
        struct GameState* gameState = passthrough;

        printf("Unhandled ping!");
}