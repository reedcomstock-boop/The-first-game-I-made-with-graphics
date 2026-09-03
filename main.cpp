#include "world.h"
#include "rooms.h"
#include "save.h"
#include "updater.h"
#include "GameLoop.h"
#include "player.h"
#include "items.h"
#include "Entity.h"
#include "Notifications.h"
#include "stats.h"
#include <iostream>
#include "raylib.h"
#include "graphics.h"
#include "sprite.h"

int main() {

    Stats startStats = {5, 5, 5, 5};
    Player player("Callum", "A runner with no memory.", 100.0, startStats);

    World world;
    world.createWorld();
    player.setLocation(world.getRoomByName("The Chute"));

    GameLoop loop(world, player);
    std::string exeDir = GetApplicationDirectory();
    ChangeDirectory(exeDir.c_str());
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "The Maze");
    SetWindowMinSize(800, 600);
    SetTargetFPS(60);

    // Load all sprite sheets — path is relative to where ./Game runs
    SpriteAnimator animator;
    animator.load("assets/sprites/tommy_walking.png",
               "assets/sprites/tommy_hurt.png",
               "assets/sprites/tommy_in_battle.png");

    std::string inputBuffer;
    std::string lastCommand;

    float playerX = 0.5f;   // fraction of scene viewport (0..1)
    float playerY = 0.1f;
    const float MOVE_SPEED = 0.9f; // fraction of viewport per second
    bool wasOnDoor = false; // edge-detection for door prompts — see the door-transition block below

    while (!WindowShouldClose() && loop.isPlaying()) {

        float dt = GetFrameTime();
        // Keep the window at the same 4:3 aspect ratio the whole layout in
        // graphics.cpp assumes (BASE_SW:BASE_SH = 800:600). Without this,
        // dragging to an extreme wide/narrow shape can make width-based and
        // height-based percentages disagree and panels overflow the window.
        if (IsWindowResized()) {
            int w = GetScreenWidth();
            const float aspect = 800.0f / 600.0f;
            int correctedH = (int)(w / aspect);
            SetWindowSize(w, correctedH);
        }
        // --- Typed input ---
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

        // If the player just confirmed "A. Yes" on a door prompt this frame,
        // apply the new position now, before movement/collision runs below
        // with a `roomName` that's about to change anyway.
        {
            float doorX, doorY;
            if (loop.consumeConfirmedDoor(doorX, doorY)) {
                playerX = doorX;
                playerY = doorY;
                wasOnDoor = false; // freshly arrived — not necessarily standing on a door here
            }
        }

        // --- Arrow keys: free movement inside the room, edge = room change ---
        float dx = 0.0f, dy = 0.0f;
        if (!loop.isPromptActive()) {
            if (IsKeyDown(KEY_LEFT))  dx -= 1.0f;
            if (IsKeyDown(KEY_RIGHT)) dx += 1.0f;
            if (IsKeyDown(KEY_UP))    dy -= 1.0f;
            if (IsKeyDown(KEY_DOWN))  dy += 1.0f;
        } else if (loop.isInDialogue()) {
            if (IsKeyPressed(KEY_UP))   loop.scrollDialogueHistory(-1);
            if (IsKeyPressed(KEY_DOWN)) loop.scrollDialogueHistory(1);
        }
        // --- Try to apply movement with collision check ---
        Room* here = player.getLocation();   // <-- move it up to here
        std::string roomName = here ? here->getName() : "";

        float nextX = playerX + dx * MOVE_SPEED * dt;
        float nextY = playerY + dy * MOVE_SPEED * dt;

        if (!isSceneTileBlocked(roomName, nextX, playerY))
            playerX = nextX;
        if (!isSceneTileBlocked(roomName, playerX, nextY))
            playerY = nextY;
        // --- Door transitions (Tiled-driven rooms only) ---
        // Edge-triggered: only fires requestDoorEntry() the frame the player
        // steps onto a door hotspot, not every frame they're standing on it —
        // otherwise declining ("B. No") would just reopen the same prompt
        // again on the very next frame.
        if (!loop.isPromptActive()) {
            TiledDoor door;
            bool onDoorNow = getSceneDoorAt(roomName, playerX, playerY, door);
            if (onDoorNow && !wasOnDoor) {
                loop.requestDoorEntry(door.targetRoom, door.spawnX, door.spawnY);
            }
            wasOnDoor = onDoorNow;
        }

        // --- NPC/monster proximity: prompt to talk within 5 tiles ---
        // Every room's Tiled grid is 23x11 (see loadTiledRoom logs) — if
        // that ever changes per-room, swap these for the actual loaded
        // room's width/height instead of the hardcoded constants.
        loop.checkNpcProximity(playerX, playerY, 23, 11);

        // Edge-of-room transitions — here is already declared above
 
        if (playerX < 0.0f) {
            if (here && here->getDestination("west")) {
                loop.runFrame("go west");
                lastCommand = "go west";
                playerX = 0.92f;
            } else {
                playerX = 0.03f;
            }
        } else if (playerX > 1.0f) {
            if (here && here->getDestination("east")) {
                loop.runFrame("go east");
                lastCommand = "go east";
                playerX = 0.08f;
            } else {
                playerX = 0.97f;
            }
        }

        if (playerY < 0.0f) {
            if (here && here->getDestination("north")) {
                loop.runFrame("go north");
                lastCommand = "go north";
                playerY = 0.85f;
            } else {
                playerY = 0.03f;
            }
        } else if (playerY > 1.0f) {
            if (here && here->getDestination("south")) {
                loop.runFrame("go south");
                lastCommand = "go south";
                playerY = 0.15f;
            } else {
                playerY = 0.97f;
            }
        }

        // --- Drive animator from game state ---
        if (player.getHealth() <= 0) {
            animator.setState(AnimState::Death);
        } else if (player.getInCombat()) {
            animator.setState(AnimState::Attack);
        } else if (dx != 0.0f || dy != 0.0f) {
            if (dy < 0)      animator.setState(AnimState::RunUp);
            else if (dy > 0) animator.setState(AnimState::RunDown);
            else if (dx > 0) animator.setState(AnimState::RunRight);
            else             animator.setState(AnimState::RunLeft);
        } else if (!lastCommand.empty() && lastCommand.substr(0,2) == "go") {
            std::string dir = lastCommand.size() > 3 ? lastCommand.substr(3) : "";
            if      (dir == "north" || dir == "up")    animator.setState(AnimState::RunUp);
            else if (dir == "south" || dir == "down")  animator.setState(AnimState::RunDown);
            else if (dir == "east")                    animator.setState(AnimState::RunRight);
            else if (dir == "west")                    animator.setState(AnimState::RunLeft);
            else                                            animator.setState(AnimState::RunDown);
            lastCommand.clear();
        } else {
            animator.setState(AnimState::IdleDown);
        }

        animator.update(dt);
        Notifications::update(dt);

        // --- Draw ---
        drawGame(world, player, inputBuffer, animator, loop.getDialogue(), playerX, playerY);
    }

    animator.unload();
    unloadSceneAssets();

    CloseWindow();
    return 0;
}