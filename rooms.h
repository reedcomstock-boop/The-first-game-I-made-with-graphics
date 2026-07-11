#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include <utility>
#include "items.h"

// Forward declarations — avoids circular include with Entity.h
class NPC;
class Monster;
class Room {
public:
    Room(const std::string& name,
         const std::string& description,
         const std::vector<std::string>& npcs,
         const std::vector<std::string>& puzzles);
    ~Room();

    std::string getName() const;
    std::string getDescription() const;
    const std::vector<Item*>& getItems() const;
    const std::vector<std::string>& getPuzzles() const;
    const std::vector<std::pair<std::string, Room*>>& getExits() const;
    const std::vector<Monster*>& getMonsterEntities() const;
    void setName(const std::string& name);
    void setDescription(const std::string& description);

    const std::vector<NPC*>& getNpcEntities() const;
    void addNpcEntity(NPC* npc);
    void removeNpcEntity(NPC* npc);
    NPC* getNpcByName(const std::string& name) const;
    Monster* getMonsterByName(const std::string& name) const;




    void addItem(Item* item);
    void removeItem(Item* item);
    void addPuzzle(const std::string& puzzle);

    void addExit(const std::string& direction, Room* destination);
    Room* getDestination(const std::string& direction) const;
    std::vector<std::string> exitNames() const;

    static void connectRooms(Room* roomA, const std::string& dirA,
                              Room* roomB, const std::string& dirB);

private:

    std::string name;
    std::string description;
    std::vector<Item*> items;
    std::vector<std::string> puzzles;
    std::vector<NPC*> npcEntities;   // Helpers, Medics, Monsters — ALL live here now
    std::vector<std::pair<std::string, Room*>> exits;
    std::vector<Monster*> monsterEntities; // Vector to hold pointers to Monster entities in the room
};
#endif // ROOM_H