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
#include "sprite.h"

int main() {
    Stats startStats = {5, 5, 5, 5};
    Player player("Thomas", "A runner with no memory.", 100.0, startStats);
 
    World world;
    world.createWorld();
    player.setLocation(world.getRoomByName("The Cage"));
 
    GameLoop loop(world, player);
 
    InitWindow(800, 600, "The Maze");
    SetTargetFPS(60);
 
    // Load all sprite sheets — path is relative to where ./Game runs
    SpriteAnimator animator;
    animator.load("assets/sprites");
 
    std::string inputBuffer;
    std::string lastCommand;
 
    while (!WindowShouldClose() && loop.isPlaying()) {
 
        float dt = GetFrameTime();
 
        // --- Input ---
        int ch = GetCharPressed();
        while (ch > 0) {
            if (ch >= 32 && ch <= 125)
                inputBuffer += (char)ch;
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !inputBuffer.empty())
            inputBuffer.pop_back();
 
        if (IsKeyPressed(KEY_ENTER)) {
            lastCommand = inputBuffer;
            loop.runFrame(inputBuffer);
            inputBuffer.clear();
        }
 
        // --- Drive animator from game state ---
        if (player.getHealth() <= 0) {
            animator.setState(AnimState::DeathDown);
        } else if (player.getInCombat()) {
            animator.setState(AnimState::AttackDown);
        } else if (!lastCommand.empty() && lastCommand.substr(0,2) == "go") {
            // Show run anim briefly after a move — resets to idle next attack/idle call
            std::string dir = lastCommand.size() > 3 ? lastCommand.substr(3) : "";
            if      (dir == "north" || dir == "up")    animator.setState(AnimState::RunUp);
            else if (dir == "south" || dir == "down")  animator.setState(AnimState::RunDown);
            else if (dir == "east")                    animator.setState(AnimState::RunRight);
            else if (dir == "west")                    animator.setState(AnimState::RunLeft);
            else                                        animator.setState(AnimState::RunDown);
            lastCommand.clear();  // clear so we go back to idle next frame
        } else {
            animator.setState(AnimState::IdleDown);
        }
 
        animator.update(dt);
 
        // --- Draw ---
        drawGame(world, player, inputBuffer, animator);
    }
 
    animator.unload();
    unloadSceneAssets();

    CloseWindow();
    return 0;
}