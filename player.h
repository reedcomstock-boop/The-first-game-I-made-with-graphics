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

    int32_t getLevel() const;      // now const, no side effects
    double getExpToNextLevel();
    double getExp() const;
    void setExp(double amt);       // level-up logic moves here    double getExp();

    void setStats(const Stats& s);

    // Used only by SaveManager to restore level/exp state directly on load,
    // bypassing the level-up side effects that setExp() triggers.
    void restoreProgress(int32_t lvl, double currentExp, double expToNextLvl);

    Tool* craftItem(const std::string& name, const std::string& description, double level, const Stats& stats, double health, double energy);

    int32_t getDiologueProgress() const ;
    void setDiologueProgress(int32_t progress) ;

    // Story-branch flags (set by combat outcomes / GameLoop, read by
    // World::createGrieverReturnEncounter and NPC dialogue stubs)
    bool getFirstGrieverDefeated() const;
    void setFirstGrieverDefeated(bool v);
    bool getBetrayedFriends() const;
    void setBetrayedFriends(bool v);
    bool getHarvestedVenom() const;
    void setHarvestedVenom(bool v);

private:
    // Additional player-specific attributes can be added here
    bool isAlive;
    bool magic;
    double maxHealth;
    double baseHealth;

    void threat(NPC* threat, CombatState& out);//make the code for this
    void attack(NPC* target, CombatState& out);//make the code for this
    void fleeComnbat(CombatState& out);//make the code for this
    void useMagic(CombatState& out); //make the code for this
    
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

    bool firstGrieverDefeated = false;
    bool betrayedFriends = false;
    bool harvestedVenom = false;
};

#endif // PLAYER_H