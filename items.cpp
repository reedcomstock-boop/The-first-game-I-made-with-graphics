#include "items.h"
#include "rooms.h"

Item::Item() : name(""), description(""), location(nullptr) {}

Item::Item(const std::string& name, const std::string& description)
    : name(name), description(description), location(nullptr) {}
Item::~Item(){}
std::string Item::getName() const {
    return name;
}

std::string Item::getDescription() const {
    return description;
}

Room* Item::getLocation() const {
    return location;
}

void Item::setName(const std::string& name) {
    this->name = name;
}

void Item::setDescription(const std::string& description) {
    this->description = description;
}

void Item::putInRoom(Room* room) {
    location = room;
    room->addItem(this);
}

Tool::Tool() : Item(), level(0), stats({0, 0, 0, 0}) {}

Tool::Tool(const std::string& name, const std::string& description, double level, const Stats& stats, double health, double energy)
    : Item(name, description), level(level), stats(stats), health(health), energy(energy) {}
Tool::~Tool(){}
Tool* Tool::getTool() const {
    return const_cast<Tool*>(this);
}
double Tool::getLevel() const {
    return level;
}

const Stats& Tool::getStats() const {
    return stats;
}
double Tool::getHealth() const{
    return health;
}
double Tool::getEnergy() const{
    return energy;
}