#include "Notifications.h"
#include <algorithm>

std::vector<Notification> Notifications::active;

void Notifications::push(const std::string& text) {
    active.push_back({text, kDisplaySeconds});
}

void Notifications::update(float dt) {
    for (auto& n : active) n.timeLeft -= dt;
    active.erase(
        std::remove_if(active.begin(), active.end(),
            [](const Notification& n) { return n.timeLeft <= 0.0f; }),
        active.end());
}

const std::vector<Notification>& Notifications::getAll() {
    return active;
}