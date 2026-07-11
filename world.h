#ifndef WORLD_H
#define WORLD_H



#include <vector>
#include <string>
#include "rooms.h"
#include "Entity.h"

class World {
public:
    World();
    ~World();

    void createWorld();
    void createWorldLvlTwo();
    void createMaze();
    void createMazePhaseTwo();

    NPC* getNpcByName(const std::string& name) const;
    Room* getRoomByName(const std::string& name) const;

    int32_t getWorldLevel() const;
    void setWorldLevel(int32_t level);
    const std::vector<Room*>& getRooms() const;

private:
    std::vector<Room*> rooms;
    int32_t worldLevel;
    Helper* newt = nullptr;
    Helper* gally = nullptr;
    Helper* minho = nullptr;
    Medic* pete = nullptr;
};

#endif // WORLD_H