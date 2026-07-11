#ifndef PLAYER_H
#define PLAYER_H
#include <vector>
#include <string>
#include "stats.h"
#include "Entity.h"
#include "rooms.h"
#include "items.h"

class Room;
class Player : public Entity {
public:
    Player(const std::string& name, const std::string& description,
           double health, const Stats& stats);
    ~Player();

    void PickUpItem(Item* item);
    void DropItem(Item* item);
    void equipItem(Tool* item);
    void unequipItem(Tool* item);
    void setLocation(Room* room);
    double MaxHealth() const;
    double MaxEnergy() const;
    Room* goDirection(const std::string& direction);
    Room* getLocation() const;
    Room* getLocationName(const std::string& name) const;

    void removeItem(const std::string& itemName);
    static std::string toLower(std::string s);
    bool hasItem(const std::string& itemName) const;
    void useTool(const std::string& toolName);
    const std::vector<Item*>& getItems() const;        // everything in the bag
    const std::vector<Item*>& getInventory() const;    // only equipped

    void useMagic();
    void setMagic(bool hasMagic) ;
    bool hasMagic() const;

    bool getInCombat() const;
    void setCombatTarget(NPC* target);
    NPC* getCombatTarget() const;
    void threat(NPC* threat);
    void attack(NPC* target);
    void fleeComnbat();
    void setInCombat(bool combat);

    int32_t getLevel() const;
    void checkLevelUp();
    double getExp();
    double getExpToNextLevel();
    void setExp(double exp);


    Tool* craftItem(const std::string& name, const std::string& description, double level, const Stats& stats, double health, double energy);

    int32_t getDiologueProgress() const ;
    void setDiologueProgress(int32_t progress) ;

private:
    // Additional player-specific attributes can be added here
    bool isAlive;
    bool magic;
    double maxHealth;
    double maxEnergy;
    double exp;
    double experienceToLevelUp;
    bool inCombat;
    int32_t level;
    int32_t dialogueProgress;
    NPC* combatTarget = nullptr;
    std::vector<Item*> inventory;                  // everything picked up
    std::vector<Item*> equippedItems;              // only what's equipped
    Room*location;
};

#endif // PLAYER_H