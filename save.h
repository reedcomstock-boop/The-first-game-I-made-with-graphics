#ifndef SAVE_H
#define SAVE_H

#include <string>

class Player;
class World;

// Very small flat-text save format (not real JSON, despite the ".json"
// default filename used by the "save"/"load" commands). Stores enough of
// Player + World state to resume: world stage, player stats/health/energy/
// level/exp, story-branch flags, current room, and inventory (including
// equipped tools). Intended to be called right after a fresh Player/World
// are constructed in main.cpp, before the game loop starts.
class SaveManager {
public:
    // Player::getExpToNextLevel() isn't const, so this takes a non-const
    // reference even though saving doesn't otherwise mutate the player.
    static bool saveGame(const std::string& filename, Player& player, const World& world);
    static bool loadGame(const std::string& filename, Player& player, World& world);
};

#endif // SAVE_H