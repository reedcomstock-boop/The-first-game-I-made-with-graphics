#include "Notifications.h"
#include <algorithm>

std::vector<Notification> Notifications::active;
OptionNotification Notifications::activeOption;

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

void Notifications::pushOption(const std::string& text, const std::vector<std::string>& options) {
    if (activeOption.active) return; // one at a time — don't stomp a pending prompt
    activeOption.active = true;
    activeOption.text = text;
    activeOption.options = options;
}

const OptionNotification& Notifications::getOption() {
    return activeOption;
}

void Notifications::clearOption() {
    activeOption = OptionNotification{};
}