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

private:
    std::string name;
    std::string description;
    Room* location;
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