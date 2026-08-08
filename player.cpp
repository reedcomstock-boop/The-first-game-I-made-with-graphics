#include "player.h"
#include "stats.h"
#include "Entity.h"
#include <algorithm>
#include <iostream>
#include <sstream>

Player::Player(const std::string& name, const std::string& description,
               double health, const Stats& stats): Entity(name, description, health, stats)
{
    isAlive = true;
    magic = false;
    maxHealth = health;
    baseHealth = health;
    maxEnergy = 100.0;
    energy = 30.0;
    exp=0.0;
    level = 0;
    experienceToLevelUp = 100.0;
    dialogueProgress = 0;
    inCombat = false;
    location = nullptr;
    inventory = {};
    equippedItems = {};
}

Player::~Player() {}

void Player::PickUpItem(Item* item) {
    inventory.push_back(item);
}

void Player::DropItem(Item* item) {
    auto it = std::find(inventory.begin(), inventory.end(), item);
    if (it != inventory.end()) {
        inventory.erase(it);
    }
}
void Player::removeItem(const std::string& itemName) {
    for (auto it = inventory.begin(); it != inventory.end(); ++it) {
        if (toLower((*it)->getName()) == toLower(itemName)) {
            inventory.erase(it);
            return;
        }
    }
}
// Returns everything in the bag
const std::vector<Item*>& Player::getItems() const {
    return inventory;
}

// Returns only equipped items
const std::vector<Item*>& Player::getInventory() const {
    return equippedItems;
}

void Player::equipItem(Tool* item) {
    applyBonus(item->getStats());
    //item->setEquipped(true);
    equippedItems.push_back(item);    // add to equipped list
}

void Player::unequipItem(Tool* item) {
    Stats negativeStats = {
        -item->getStats().strength,
        -item->getStats().dexterity,
        -item->getStats().intelligence,
        -item->getStats().defence
    };
    applyBonus(negativeStats);
    //item->setEquipped(false);
    // remove from equipped list
    auto it = std::find(equippedItems.begin(), equippedItems.end(), item);
    if (it != equippedItems.end()) {
        equippedItems.erase(it);
    }

}
void Player::setLocation(Room* room){
    location = room;

}
double Player::MaxHealth() const {
    return baseHealth + 5 * getStats().defence; // Assuming health is part of Stats
}
double Player::MaxEnergy() const {
    double lvl = (level > 0) ? static_cast<double>(level) : 1.0;
    return lvl + (20 * getStats().intelligence); // Assuming energy is part of Stats
}
double Player::getExp() const{
    return exp;
}
double Player::getExpToNextLevel(){
    return experienceToLevelUp;
}
Room* Player::goDirection(const std::string& direction) {
    if (location) {
        Room* nextRoom = location->getDestination(direction);
        if (nextRoom) {
            setLocation(nextRoom);
            return nextRoom;
        }
    }
    return nullptr; // No valid exit in that direction
}
Room* Player::getLocation() const {
    return location;
}

Room* Player::getLocationName(const std::string& name) const {
    if (location && location->getName() == name) {
        return location;
    }
    return nullptr;
}
int32_t Player::getDiologueProgress() const {
    return dialogueProgress; 
    }
void Player::setDiologueProgress(int32_t progress) { 
    dialogueProgress = progress; 
    }
void Player::setExp(double amt) {
    exp += amt;
    std::cout << "\nYou gained " << amt << " experience points!\n";
    while (exp >= experienceToLevelUp) {
        exp -= experienceToLevelUp;
        experienceToLevelUp *= 1.5;
        level++;
        std::cout << "\nYou leveled up! You are now level " << level << ".\n";
    }
}

int32_t Player::getLevel() const {
    return level;
}

void Player::setInCombat(bool combat) {
    inCombat = combat;
}
bool Player::getInCombat() const { 
    return inCombat; 
}
NPC* Player::getCombatTarget() const { 
    return combatTarget; 
}
void Player::setCombatTarget(NPC* target) { 
    combatTarget = target; 
}
void Player::attack(NPC* target, CombatState& out) {
    if (!inCombat || !target) return;
    out.target = target;
    out.playerAnim = "attack";

    double damage = getStats().strength - target->getStats().defence;
    if (damage < 0) damage = 0;
    target->setHealth(target->getHealth() - damage);
    out.pushLog("You attack " + target->getName() + " for " + std::to_string((int)damage) + " damage.");
    out.enemyMaxHealth = target->getMaxHealth();

    if (target->getHealth() <= 0) {
        out.enemyHealth = 0;
        out.enemyAnim = (rand() % 2 == 0) ? "Die1" : "Die2";
        out.pushLog(target->getName() + " has been defeated!");
        if (target->getName() == "The First Griever") firstGrieverDefeated = true;
        setExp(target->getExperience());
        inCombat = false; combatTarget = nullptr;
        out.active = false;
        return;
    }

    out.enemyHealth = target->getHealth();
    out.enemyAnim = "Hit";

    double targetDamage = target->getStats().strength - getStats().defence;
    if (targetDamage < 0) targetDamage = 0;
    setHealth(getHealth() - targetDamage);
    out.playerHealth = getHealth();
    out.pushLog(target->getName() + " attacks you for " + std::to_string((int)targetDamage) + " damage.");

    if (getHealth() <= 0) {
        out.pushLog("You have been defeated by " + target->getName() + "!");
        isAlive = false; inCombat = false; combatTarget = nullptr;
        out.active = false;
        return;
    }
    out.active = true;
}

void Player::useMagic(CombatState& out) {
    if (!magic) { out.pushLog("You don't know any magic yet."); return; }
    if (!inCombat || !combatTarget) return;

    out.playerAnim = "attack";
    out.enemyAnim  = "Dizzy";
    out.pushLog("You unleash a pulse of energy at " + combatTarget->getName() + "!");
    out.pushLog(combatTarget->getName() + " reels, dazed, and the fight breaks off.");

    if (combatTarget->getName() == "The First Griever") firstGrieverDefeated = true;
    setExp(combatTarget->getExperience());

    inCombat = false; combatTarget = nullptr;
    out.active = false;
}

void Player::fleeComnbat(CombatState& out) {
    if (!inCombat || !combatTarget) { out.pushLog("You are not in combat."); return; }
    out.pushLog("You attempt to flee from combat...");
    double avoidChance = (getStats().dexterity + getStats().intelligence) / 2.0;
    int roll = rand() % 10;
    NPC* fledFrom = combatTarget;

    if (roll >= avoidChance) {
        out.pushLog("You successfully fled from " + fledFrom->getName() + "!");
        if (fledFrom->getName() == "The First Griever") {
            betrayedFriends = true;
            out.pushLog("You leave Alby and Minho to face it alone...");
        }
        inCombat = false; combatTarget = nullptr;

        // Fleeing draws attention — any *other* live monster in the room gets
        // one chance to notice and jump you before you actually leave.
        if (location) {
            for (Monster* m : location->getMonsterEntities()) {
                if (m == fledFrom || m->getHealth() <= 0) continue;
                double watch = (getStats().dexterity + getStats().intelligence) / 2.0;
                if ((rand() % 10) >= watch) {
                    out.pushLog(m->getName() + " notices you trying to leave!");
                    threat(m, out); // re-engages: sets inCombat, combatTarget, out.active, snapshots enemy HP
                    return;
                }
            }
        }
        out.active = false;
    } else {
        out.pushLog("You failed to flee! " + fledFrom->getName() + " attacks you!");
        out.enemyAnim = "Attack1";
        double targetDamage = fledFrom->getStats().strength - getStats().defence;
        if (targetDamage < 0) targetDamage = 0;
        setHealth(getHealth() - targetDamage);
        out.playerHealth = getHealth();
        if (getHealth() <= 0) {
            out.pushLog("You have been defeated by " + fledFrom->getName() + "!");
            isAlive = false; inCombat = false; combatTarget = nullptr;
            out.active = false;
            return;
        }
        out.active = true;
    }
}

void Player::threat(NPC* t, CombatState& out) {
    setInCombat(true);
    combatTarget = t;
    out.active = true;
    out.target = t;
    out.playerAnim = "idle";
    out.enemyAnim  = "Idle_Nervous";
    out.playerHealth = getHealth();
    out.playerMaxHealth = MaxHealth();
    out.enemyHealth = t->getHealth();
    out.enemyMaxHealth = t->getMaxHealth();
    out.pushLog("You are now in combat with " + t->getName() + "!");
}

void Player::useTool(const std::string& toolName) {
    Tool* tool = nullptr;
    for (auto item : inventory) {
        if (toLower(item->getName()) == toLower(toolName)) {
            tool = dynamic_cast<Tool*>(item);
            break; // stop searching once we find a name match, tool or not
        }
    }
    if (!tool) {
        std::cout << "You don't have a tool called '" << toolName << "' in your inventory.\n";
        return;
    }
    setHealth(getHealth() + tool->getHealth());
    setEnergy(getEnergy() + tool->getEnergy());
    std::cout << "You used '" << toolName << "', your health is " << getHealth()
               << " and your energy is " << getEnergy() << ".\n";
}
void Player::setMagic(bool hasMagic) {
    magic = hasMagic;
}
bool Player::hasMagic() const {
    return magic;
}
void Player::restoreProgress(int32_t lvl, double currentExp, double expToNextLvl) {
    level = lvl;
    exp = currentExp;
    experienceToLevelUp = expToNextLvl;
}
void Player::setStats(const Stats& s) {
    applyBonus(Stats{
        s.strength - getStats().strength,
        s.dexterity - getStats().dexterity,
        s.intelligence - getStats().intelligence,
        s.defence - getStats().defence
    });
}

std::string Player::toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
bool Player::hasItem(const std::string& itemName) const {
    for (const auto& item : inventory) {
        if (toLower(item->getName()) == toLower(itemName)) {
            return true;
        }
    }
    return false;
}
Tool* Player::craftItem(const std::string& name, const std::string& description, double level, const Stats& stats, double health, double energy) {
    // Implement crafting logic here
    return new Tool(name, description, level, stats, health, energy);
}

bool Player::getFirstGrieverDefeated() const { return firstGrieverDefeated; }
void Player::setFirstGrieverDefeated(bool v) { firstGrieverDefeated = v; }
bool Player::getBetrayedFriends() const { return betrayedFriends; }
void Player::setBetrayedFriends(bool v) { betrayedFriends = v; }
bool Player::getHarvestedVenom() const { return harvestedVenom; }
void Player::setHarvestedVenom(bool v) { harvestedVenom = v; }