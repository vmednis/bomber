#include <stdio.h>
#include <raylib.h>
#include "../client/tests.h"

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

unsigned char worldX = 9;
unsigned char worldY = 7;
unsigned char world[7][9] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 2, 0, 0, 0, 1},
        {1, 0, 1, 2, 1, 2, 1, 0, 1},
        {1, 0, 2, 0, 2, 0, 2, 0, 1},
        {1, 2, 1, 0, 1, 0, 1, 2, 1},
        {1, 0, 0, 0, 2, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1}
};

Texture2D walls[16] = {0};

int main() {
        int i, j;
        SelfTest();

        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bomber Client");

        walls[1] = LoadTexture("assets/wall_solid.png");
        walls[2] = LoadTexture("assets/wall_breakable.png");

        while(!WindowShouldClose()) {
                BeginDrawing();
                        ClearBackground(RAYWHITE);
                        DrawTexture(walls[1], 0, 0, WHITE);
                        for(i = 0; i < worldY; i++) {
                                for(j = 0; j < worldX; j++) {
                                        if(world[i][j] != 0) {
                                                DrawTexture(walls[world[i][j]], j * 64, i * 64, WHITE);
                                        }
                                }
                        }
                EndDrawing();
        }

        CloseWindow();
        return 0;
}
