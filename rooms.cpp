#include "rooms.h"
#include "Entity.h"
#include <algorithm>
#include <iostream>
#include <cctype>

Room::Room(const std::string& name,
           const std::string& description,
           const std::vector<std::string>&,
           const std::vector<std::string>& puzzles)
    : name(name), description(description), puzzles(puzzles)
{
}

Room::~Room()
{
    for (Item* item : items) {
        delete item;
    }
    for (NPC* npc : npcEntities) {
        delete npc;
    }
}

std::string Room::getName() const
{
    return name;
}

std::string Room::getDescription() const
{
    return description;
}

const std::vector<Item*>& Room::getItems() const
{
    return items;
}

const std::vector<Monster*>& Room::getMonsterEntities() const {
    return monsterEntities;
}

Monster* Room::getMonsterByName(const std::string& name) const {
    for (Monster* m : monsterEntities) {
        if (m->getName() == name) return m;
    }
    return nullptr;
}
const std::vector<std::string>& Room::getPuzzles() const
{
    return puzzles;
}

const std::vector<std::pair<std::string, Room*>>& Room::getExits() const
{
    return exits;
}

void Room::setName(const std::string& name)
{
    this->name = name;
}

void Room::setDescription(const std::string& description)
{
    this->description = description;
}

void Room::addItem(Item* item)
{
    items.push_back(item);
}

void Room::removeItem(Item* item)
{
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (*it == item) {
            items.erase(it);
            return;
        }
    }
}


const std::vector<NPC*>& Room::getNpcEntities() const {
    return npcEntities;
}
void Room::addNpcEntity(NPC* npc) {
    npcEntities.push_back(npc);
}
NPC* Room::getNpcByName(const std::string& name) const {
    std::string target = name;
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    for (NPC* npc : npcEntities) {
        std::string npcName = npc->getName();
        std::transform(npcName.begin(), npcName.end(), npcName.begin(), ::tolower);
        if (npcName == target) return npc;
    }
    return nullptr;
}
void Room::removeNpcEntity(NPC* npc)
{
    for (auto it = npcEntities.begin(); it != npcEntities.end(); ++it) {
        if (*it == npc) {
            npcEntities.erase(it);
            return;
        }
    }
}

void Room::addPuzzle(const std::string& puzzle)
{
    puzzles.push_back(puzzle);
}

void Room::addExit(const std::string& direction, Room* destination)
{
    for (const auto& e : exits) {
        if (e.first == direction) {
            return;
        }
    }
    exits.push_back({direction, destination});
}

Room* Room::getDestination(const std::string& direction) const
{
    for (const auto& e : exits) {
        if (e.first == direction) {
            return e.second;
        }
    }
    return nullptr;
}

std::vector<std::string> Room::exitNames() const
{
    std::vector<std::string> names;
    for (const auto& e : exits) {
        names.push_back(e.first);
    }
    return names;
}

void Room::connectRooms(Room* roomA, const std::string& dirA,
                         Room* roomB, const std::string& dirB)
{
    roomA->addExit(dirA, roomB);
    roomB->addExit(dirB, roomA);
}