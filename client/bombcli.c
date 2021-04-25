#include <stdio.h>
#include <raylib.h>
#include "../client/tests.h"
#include "../client/gamestate.h"
#include "../client/texture_atlas.h"

#define UNUSED __attribute__((unused))
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

void LoadTextures(Texture2D* atlas);
void DrawFrame(struct GameState* gameState, Texture2D* textureAtlas, float delta);

int main() {
        float delta;
        struct GameState gameState = {0};
        Texture2D textureAtlas[TEXTURE_ATLAS_SIZE] = {0};

        SelfTest();

        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bomber Client");
        LoadTextures(textureAtlas);

        while(!WindowShouldClose()) {
                delta = GetFrameTime();

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

        for(i = 0; i < gameState->worldY; i++) {
                for(j = 0; j < gameState->worldX; j++) {
                        tile = gameState->world[i][j];
                        if(tile != 0) {
                                DrawTexture(textureAtlas[tile], j * 64, i * 64, WHITE);
                        }
                }
        }
}