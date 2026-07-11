#ifndef UPDATER_H
#define UPDATER_H

#include <vector>

class Updatable {
public:
    virtual void update() = 0;
    virtual ~Updatable() {}
};

class Updater {
public:
    void registerEntity(Updatable* entity);
    void deregisterEntity(Updatable* entity);
    void updateAll();

private:
    std::vector<Updatable*> entities;
};

#endif // UPDATER_H