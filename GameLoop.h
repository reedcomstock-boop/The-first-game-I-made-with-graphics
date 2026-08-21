#ifndef GAMELOOP_H
#define GAMELOOP_H
#include <iostream>
#include <sstream>
#include <algorithm>
#include "world.h"
#include "Entity.h" 
#include "player.h"
#include "save.h"

class GameLoop {
public:
    GameLoop(World& world, Player& player);
    void runFrame(const std::string& input); // process one command if input is ready
    bool isPlaying() const;
    const DialogueState& getDialogue() const { return dialogue; }
    bool isInDialogue() const { return dialogue.active; }
    void scrollDialogueHistory(int delta);   // delta: -1 = older, +1 = newer

    // True while dialogue OR an option notif (door confirm, etc.) is up —
    // main.cpp uses this to freeze player movement instead of isInDialogue()
    // alone, since option notifs don't set dialogue.active.
    bool isPromptActive() const;

    // Doors: main.cpp calls this once when the player first steps onto a
    // door hotspot (edge-triggered, not every frame) instead of teleporting
    // immediately. Shows an "option notif" A/B confirm prompt. No-ops if a
    // dialogue/prompt is already open.
    void requestDoorEntry(const std::string& targetRoomName, float spawnX, float spawnY);

    // main.cpp should call this once per frame, after runFrame() has had a
    // chance to process an Enter keypress. Returns true exactly once, the
    // frame the player picks "A. Yes" — outX/outY are where to place the
    // player sprite in the new room. Returns false every other frame,
    // including when the player picks "B. No" (nothing to consume then).
    bool consumeConfirmedDoor(float& outX, float& outY);
private:
    World&  world;
    Player& player;
    bool playing;
    DialogueState dialogue;
    CombatState combat;

    void printSituation() const;
    void showHelp() ;

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
    bool cmdMe();
    bool cmdCraft(const std::string& itemName);
    bool cmdSave(const std::string& filename);
    bool cmdLoad(const std::string& filename);

    bool handleDialogueChoice(const std::string& input);
    bool handleOptionNotifChoice(const std::string& input);
    bool checkWorldProgression(); // returns true if the world advanced a stage

    // Door-confirm prompt state — see requestDoorEntry()/consumeConfirmedDoor().
    bool doorPromptActive = false;
    std::string pendingDoorTarget;
    float pendingSpawnX = 0.5f, pendingSpawnY = 0.5f;
    bool doorJustConfirmed = false;
    float confirmedSpawnX = 0.5f, confirmedSpawnY = 0.5f;
};
#endif // GAMELOOP_H