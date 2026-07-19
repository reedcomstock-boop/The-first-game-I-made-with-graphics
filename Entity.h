#ifndef ENTITY_H
#define ENTITY_H

#include <cstdint>
#include <string>
#include "stats.h"
#include "updater.h"
class Player;
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
    double energy;  // Energy level for NPCs, decreases over time
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
    virtual void talk(Player& player);

};

class Helper : public NPC {
public:
    Helper(const std::string& name, const std::string& description, double health, const Stats& stats);
    void update() override;
    void talk(Player& player) override;
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
    void talk(Player& player) override;

};

#endif // ENTITY_H