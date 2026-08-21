#ifndef ITEMS_H
#define ITEMS_H

#include <string>
#include "stats.h"

class Room;
class Item {
public:
    virtual ~Item();
    Item(const std::string& name, const std::string& description);
    Item();
    std::string getName() const;
    std::string getDescription() const;
    Room* getLocation() const;

    void setName(const std::string& name);
    void setDescription(const std::string& description);

    void putInRoom(Room* room);

    // Optional on-screen position, as a fraction (0..1) of the room's scene
    // viewport — same convention as door spawn points and prop placement.
    // Unset by default (hasPosition()==false), meaning the item only shows
    // up in the text "Items" list like before and draws nothing in the
    // scene. Call setPosition() to also place a visible icon in the room,
    // e.g. next to a crate tile.
    void setPosition(float relX, float relY);
    bool hasPosition() const;
    float getPosX() const;
    float getPosY() const;

private:
    std::string name;
    std::string description;
    Room* location;
    bool positionSet = false;
    float posX = 0.0f, posY = 0.0f;
};

class Tool : public Item {
public:
    Tool();
    Tool(const std::string& name, const std::string& description, double level, const Stats& stats, double health, double energy);
    ~Tool();
    double getLevel() const;
    const Stats& getStats() const;
    Tool* getTool() const ;
    double getHealth()const;
    double getEnergy()const;

private:
    double level;
    Stats stats;
    double health;
    double energy;
};

#endif // ITEMS_H