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
struct CombatState {
    bool active = false;
    NPC* target = nullptr;

    std::string playerAnim = "idle"; // "idle" | "attack" | "death" -> mapped to Thomas's SpriteAnimator states
    std::string enemyAnim  = "idle"; // exact StripAnimator clip name: Attack1/2/3, Hit, Dizzy, Die1/2, Idle_Nervous

    double playerHealth = 0.0, playerMaxHealth = 0.0;
    double enemyHealth  = 0.0, enemyMaxHealth  = 0.0;

    std::vector<std::string> log;
    static const size_t kMaxLog = 6;
    void pushLog(const std::string& line) {
        log.push_back(line);
        while (log.size() > kMaxLog) log.erase(log.begin());
    }
    void clear() {
        active = false; target = nullptr;
        playerAnim = "idle"; enemyAnim = "idle";
        log.clear();
        playerHealth = playerMaxHealth = enemyHealth = enemyMaxHealth = 0.0;
    }
};
struct DialogueState {
    bool active = false;
    bool clear_after = false;

    std::string speaker;
    std::vector<std::string> lines;    // the NPC's most recent lines (current page)
    std::vector<std::string> options;  // choices for the CURRENT page only
    NPC* npc = nullptr;

    // Full transcript of everything said so far in this conversation, as
    // "Speaker: line" strings, so the player can scroll back if they miss
    // something. Reset whenever a new conversation starts (see clear()).
    std::vector<std::string> history;
    int scrollOffset = 0;   // 0 = viewing the live/current page; >0 = scrolled back

    void clear() {
        active = false;
        clear_after = false;
        speaker.clear();
        lines.clear();
        options.clear();
        npc = nullptr;
        history.clear();
        scrollOffset = 0;
    }

    // Call right after lines/speaker are set by talk()/continueTalk(), so
    // the transcript always includes whatever's currently on screen too.
    void recordHistory() {
        for (const auto& line : lines) {
            history.push_back(speaker + ": " + line);
        }
        scrollOffset = 0;   // any new line snaps the view back to "live"
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
    virtual void talk(Player& player, DialogueState& out);
    virtual void continueTalk(Player& player, DialogueState& out, int choiceIndex);
    double getMaxHealth() const { return maxHealth; }

protected:
    double maxHealth;
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