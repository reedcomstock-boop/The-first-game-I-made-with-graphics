#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H
#include <string>
#include <vector>

// A single on-screen announcement — level ups, item pickups, world-stage
// changes, etc. Lives for kDisplaySeconds after being pushed, then expires
// automatically. Shared across the whole game (same static pattern as
// Updater's game clock), so Player/World/GameLoop can all push messages
// without needing a reference threaded through their call signatures.
struct Notification {
    std::string text;
    float timeLeft;
};

class Notifications {
public:
    static void push(const std::string& text);
    static void update(float dt);              // call once per frame
    static const std::vector<Notification>& getAll();

private:
    static std::vector<Notification> active;
    static constexpr float kDisplaySeconds = 3.0f;
};

#endif // NOTIFICATIONS_H