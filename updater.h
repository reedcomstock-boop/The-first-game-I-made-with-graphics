// updater.h
#ifndef UPDATER_H
#define UPDATER_H

#include <vector>
#include <cstdint>

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
    static int32_t getGameClock();
    static int32_t getUpdateCount();
    static void resetUpdateCount();
    static void incrementUpdateCount();
    static void convertUpdatesIntoGameClock();

private:
    std::vector<Updatable*> entities;   // stays per-instance — fine as-is
    static int32_t updateCount;         // shared across the whole game
    static int32_t gameClock;
};

#endif // UPDATER_H