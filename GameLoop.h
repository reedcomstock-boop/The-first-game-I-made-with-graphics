#ifndef GAMELOOP_H
#define GAMELOOP_H
#include <iostream>
#include <sstream>
#include <algorithm>
#include "world.h"
#include "player.h"

class GameLoop {
public:
    GameLoop(World& world, Player& player);
    void runFrame(const std::string& input); // process one command if input is ready
    bool isPlaying() const;  
    //void run();

private:
    World&  world;
    Player& player;
    bool playing;

    void printSituation() const;
    void showHelp() const;

    // Command handlers
    bool cmdGo(const std::string& direction);
    bool cmdPickup(const std::string& itemName);
    bool cmdDrop(const std::string& itemName);
    bool cmdInventory() const;
    bool cmdUseTool(const std::string& itemName);
    bool cmdEquip(const std::string& itemName);
    bool cmdUnequip(const std::string& itemName);
    bool cmdAttack(const std::string& targetName);
    bool cmdFlee();
    bool cmdUseMagic();
    bool cmdTalk(const std::string& npcName);
    bool cmdLook() const;
    bool cmdMe() const;

    void checkWorldProgression();
};
#endif // GAMELOOP_H
