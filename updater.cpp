// updater.cpp
#include "updater.h"
#include <algorithm>

int32_t Updater::updateCount = 0;   // static members need an out-of-line definition
int32_t Updater::gameClock   = 0;

void Updater::registerEntity(Updatable* entity) {
    entities.push_back(entity);
}

void Updater::deregisterEntity(Updatable* entity) {
    auto it = std::find(entities.begin(), entities.end(), entity);
    if (it != entities.end()) {
        entities.erase(it);
    }
}

void Updater::convertUpdatesIntoGameClock() {
    if (updateCount > 3) {
        gameClock++;
        updateCount -= 3;
    }
}
int32_t Updater::getUpdateCount() {
    return updateCount;
}
void Updater::resetUpdateCount() {
    updateCount = 0;
}
void Updater::incrementUpdateCount() {
    updateCount++;
}
void Updater::updateAll() {
     
    for (Updatable* entity : entities) {
        entity->update();
    }
}