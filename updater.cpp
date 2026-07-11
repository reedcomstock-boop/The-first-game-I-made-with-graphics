#include "updater.h"
#include <algorithm>

void Updater::registerEntity(Updatable* entity) {
    entities.push_back(entity);
}

void Updater::deregisterEntity(Updatable* entity) {
    auto it = std::find(entities.begin(), entities.end(), entity);
    if (it != entities.end()) {
        entities.erase(it);
    }
}

void Updater::updateAll() {
    for (Updatable* entity : entities) {
        entity->update();
    }
}