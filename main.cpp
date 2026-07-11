#include "world.h"
#include "rooms.h"
//#include "save.h"
#include "updater.h"
#include "GameLoop.h"
#include "player.h"
#include "items.h"
#include "Entity.h"
#include "stats.h"
#include <iostream>
#include "raylib.h"
#include "graphics.h"

int main() {
    Stats startStats = {5, 5, 5, 5};
    Player player("Thomas", "A runner with no memory.", 100.0, startStats);

    World world;
    world.createWorld();

    player.setLocation(world.getRoomByName("The Cage"));

    GameLoop loop(world, player);
    InitWindow(800, 600, "The Maze");
    SetTargetFPS(60);

    std::string inputBuffer; // what the player is typing right now

    while (!WindowShouldClose() && loop.isPlaying()) {

        // collect keypresses
        int ch = GetCharPressed();
        while (ch > 0) {
            if (ch >= 32 && ch <= 125)
                inputBuffer += (char)ch;
            ch = GetCharPressed();
        }

        // backspace
        if (IsKeyPressed(KEY_BACKSPACE) && !inputBuffer.empty())
            inputBuffer.pop_back();

        // enter — send command
        if (IsKeyPressed(KEY_ENTER)) {
            loop.runFrame(inputBuffer);
            inputBuffer.clear();
        }

        // draw
        drawGame(world, player, inputBuffer);
    }

CloseWindow();

    return 0;
}