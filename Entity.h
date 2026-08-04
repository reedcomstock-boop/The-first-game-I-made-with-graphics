#ifndef ENTITY_H
#define ENTITY_H

#include <cstdint>
#include <string>
#include <vector>
#include "stats.h"
#include "updater.h"
class Player;
class NPC;
// Shared, non-blocking dialogue state. GameLoop owns one instance; NPC::talk
// implementations fill it in and return immediately instead of blocking on
// std::cin. graphics.cpp draws it; main.cpp/GameLoop feed player choices
// into it from the input buffer.
struct DialogueState {
    bool active = false;
    bool clear_after = false;         // whether to clear the dialogue after this one

    std::string speaker;
    std::vector<std::string> lines;    // lines to show right now (paged)
    std::vector<std::string> options;  // e.g. {"A. ...", "B. ..."} — empty if no choice pending
    NPC* npc = nullptr;                // who we're mid-conversation with (for resuming)

    void clear() {
        active = false;
        clear_after = false;
        speaker.clear();
        lines.clear();
        options.clear();
        npc = nullptr;
    }
};

class Entity : public Updatable {
public:
    Entity(const std::string& name, const std::string& description,
           double health, const Stats& stats);
    static std::string toLower(std::string s);
    std::string getName() const;
    std::string getDescription() const;
    double getHealth() const;
    const Stats& getStats() const;

    void update() override;

    void setName(const std::string& name);
    void setDescription(const std::string& description);
    void setHealth(double health);
    void applyBonus(const Stats& bonus);
    double getExperience() const;
    double getEnergy()const;
    void setEnergy(double Energy);
    int32_t getDialogueProgress() const;
    void setDialogueProgress(int32_t lvl);

protected:
    double energy;
    double experience;
    int32_t dialogueProgress;

private:
    std::string name;
    std::string description;
    double health;
    int32_t level;
    Stats stats;
};

class NPC : public Entity {
public:
    NPC(const std::string& name, const std::string& description, double health, const Stats& stats);
    void update() override;
    // talk() now fills `out` and returns immediately — no blocking I/O.
    virtual void talk(Player& player, DialogueState& out);
    // Called when the player picks an option while mid-conversation with
    // this NPC. `choiceIndex` is 0-based (A=0, B=1, ...).
    virtual void continueTalk(Player& player, DialogueState& out, int choiceIndex);
};

class Helper : public NPC {
public:
    Helper(const std::string& name, const std::string& description, double health, const Stats& stats);
    void update() override;
    void talk(Player& player, DialogueState& out) override;
    void continueTalk(Player& player, DialogueState& out, int choiceIndex) override;
};

class Monster : public NPC {
public:
    Monster(const std::string& name, const std::string& description, double health, const Stats& stats, double exp);
    void update() override;
private:
};

class Medic : public NPC {
public:
    Medic(const std::string& name, const std::string& description, double health, const Stats& stats);
    void update() override;
    void talk(Player& player, DialogueState& out) override;
    void continueTalk(Player& player, DialogueState& out, int choiceIndex) override;
};

#endif // ENTITY_H