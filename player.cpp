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
    maxEnergy = 100.0;
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
    return getHealth() + 5 * getStats().defence; // Assuming health is part of Stats
}
double Player::MaxEnergy() const {
    double lvl = (level > 0) ? static_cast<double>(level) : 1.0;
    return lvl + (20 * getStats().intelligence); // Assuming energy is part of Stats
}
double Player::getExp(){
    return exp;
}
double Player::getExpToNextLevel(){
    return experienceToLevelUp;
}
void Player::setExp(double amt){
    exp += amt;
    std::cout << "You gained " << amt << " experience points!\n";
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

int32_t Player::getLevel() const {
    return level;
}
void Player::checkLevelUp() {
    if (exp >= experienceToLevelUp) {
        exp -= experienceToLevelUp;
        experienceToLevelUp *= 1.5;
        level++;
        std::cout << "\nYou leveled up! You are now level " << level << ".\n";
    }
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

void Player::attack(NPC* target) {
    if (!inCombat || !target) return;

    double damage = getStats().strength - target->getStats().defence;
    if (damage < 0){
        damage = 0;
    }
    target->setHealth(target->getHealth() - damage);
    std::cout << "You attack " << target->getName() << " for " << damage << " damage.\n";

    if (target->getHealth() <= 0) {
        std::cout << target->getName() << " has been defeated!\n";
        setExp(target->getExperience());
        inCombat = false;
        combatTarget = nullptr;
        return;
    }

    // Enemy retaliates — one round only
    double targetDamage = target->getStats().strength - getStats().defence;
    if (targetDamage < 0) targetDamage = 0;
    setHealth(getHealth() - targetDamage);
    std::cout << target->getName() << " attacks you for " << targetDamage << " damage.\n";

    if (getHealth() <= 0) {
        std::cout << "You have been defeated by " << target->getName() << "!\n";
        isAlive = false;
        inCombat = false;
        combatTarget = nullptr;
    }
}

void Player::threat(NPC* threat) {
    setInCombat(true);
    combatTarget = threat;
    std::cout << "You are now in combat with " << threat->getName() << "!\n";
    // No auto-attack here anymore — just starts the encounter.
    // The player will choose attack/flee/magic on their next turn.
}

void Player::fleeComnbat() {
    if (!inCombat || !combatTarget) {
        std::cout << "You are not in combat.\n";
        return;
    }
    std::cout << "You attempt to flee from combat...\n";
    double avoidChance = (getStats().dexterity + getStats().intelligence) / 2.0;
    int roll = rand() % 10;
    if (roll >= avoidChance) {
        std::cout << "You successfully fled from combat!\n";
        inCombat = false;
        combatTarget = nullptr;
    } else {
        std::cout << "You failed to flee! " << combatTarget->getName() << " attacks you!\n";
        double targetDamage = combatTarget->getStats().strength - getStats().defence;
        if (targetDamage < 0) targetDamage = 0;
        setHealth(getHealth() - targetDamage);
        if (getHealth() <= 0) {
            std::cout << "You have been defeated by " << combatTarget->getName() << "!\n";
            isAlive = false;
            inCombat = false;
            combatTarget = nullptr;
        }
    }
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
void Player::useMagic() {
    if (!magic) {
        std::cout << "You don't know any magic yet.\n";
        return;
    }
    if (!inCombat || !combatTarget) return;

    double damage = getStats().intelligence * 2 - combatTarget->getStats().defence;
    if (damage < 0) damage = 0;
    combatTarget->setHealth(combatTarget->getHealth() - damage);
    std::cout << "You cast a spell on " << combatTarget->getName() << " for " << damage << " damage!\n";

    if (combatTarget->getHealth() <= 0) {
        std::cout << combatTarget->getName() << " has been defeated!\n";
        setExp(combatTarget->getExperience());
        inCombat = false;
        combatTarget = nullptr;
        return;
    }

    double targetDamage = combatTarget->getStats().strength - getStats().defence;
    if (targetDamage < 0) targetDamage = 0;
    setHealth(getHealth() - targetDamage);
    std::cout << combatTarget->getName() << " attacks you for " << targetDamage << " damage.\n";

    if (getHealth() <= 0) {
        std::cout << "You have been defeated by " << combatTarget->getName() << "!\n";
        isAlive = false;
        inCombat = false;
        combatTarget = nullptr;
    }
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