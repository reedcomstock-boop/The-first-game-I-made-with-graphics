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

// "option notif" — a blocking Yes/No (or multi-choice) prompt drawn the
// same way as a regular notification, but it doesn't auto-expire; it waits
// for the player to answer. Used for things like door confirmations that
// shouldn't take over the whole screen the way NPC dialogue does. Only one
// can be active at a time — pushOption() while one is already active does
// nothing, same spirit as Notifications::push() just queuing normally.
struct OptionNotification {
    bool active = false;
    std::string text;
    std::vector<std::string> options; // e.g. {"A. Yes", "B. No"}
};

class Notifications {
public:
    static void push(const std::string& text);
    static void update(float dt);              // call once per frame
    static const std::vector<Notification>& getAll();

    static void pushOption(const std::string& text, const std::vector<std::string>& options);
    static const OptionNotification& getOption();
    static void clearOption();   // call once the player has answered

private:
    static std::vector<Notification> active;
    static OptionNotification activeOption;
    static constexpr float kDisplaySeconds = 3.0f;
};

#endif // NOTIFICATIONS_H