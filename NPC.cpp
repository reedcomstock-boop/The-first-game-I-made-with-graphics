#include "Entity.h"
#include "player.h"
#include "rooms.h"
#include "GameLoop.h"
#include <iostream>
#include <sstream>
#include <algorithm>

std::string Entity::toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

Entity::Entity(const std::string& name, const std::string& description,
               double health, const Stats& stats)
    : name(name), description(description), health(health), stats(stats) {
    energy = 0;
    experience = 0;
    level = 1;
    dialogueProgress = 0;
}

std::string Entity::getName() const { return name; }
std::string Entity::getDescription() const { return description; }
double Entity::getHealth() const { return health; }
const Stats& Entity::getStats() const { return stats; }
void Entity::setName(const std::string& n) { this->name = n; }
void Entity::setDescription(const std::string& d) { this->description = d; }
void Entity::setHealth(double h) { this->health = h; }
double Entity::getEnergy() const { return energy; }
void Entity::setEnergy(double e) { this->energy = e; }
void Entity::applyBonus(const Stats& bonus) { stats += bonus; }
double Entity::getExperience() const { return experience; }
int32_t Entity::getDialogueProgress() const { return dialogueProgress; }
void Entity::setDialogueProgress(int32_t progress) { dialogueProgress = progress; }

void Entity::update() {
    // Base entities (Player) don't do anything automatically on update;
    // NPC subclasses override this with their own behavior.
}

// ---------------------------------------------------------------- NPC ----
NPC::NPC(const std::string& name, const std::string& description,
         double health, const Stats& stats)
    : Entity(name, description, health, stats) {}

void NPC::update() {
    energy -= 1;
    if (energy <= 1) {
        setHealth(getHealth() - 1);
    }
}
void NPC::talk(Player&, DialogueState& out) {
    out.active = true;
    out.speaker = getName();
    out.lines = { getName() + " has nothing to say." };
    out.options.clear();
    out.npc = this;
}
void NPC::continueTalk(Player&, DialogueState& out, int) {
    out.clear();
}

// ------------------------------------------------------------- Helper ----
Helper::Helper(const std::string& name, const std::string& description,
               double health, const Stats& stats)
    : NPC(name, description, health, stats) {}

void Helper::update() {
    NPC::update();
}

// Local sub-step counters, one per named Helper, so multi-question chats
// within a single dialogueProgress/level stage can pause mid-conversation.
// Keyed by NPC pointer so Gally/Minho/Terrisa/Alby don't share state.
#include <unordered_map>
static std::unordered_map<NPC*, int> g_subStep;

void Helper::talk(Player& player, DialogueState& out) {
    std::string n = getName();
    out.active = true;
    out.speaker = n;
    out.npc = this;
    out.options.clear();
    g_subStep[this] = 0;  // fresh conversation entry point

    if (toLower(n) == "gally") {
        out.lines = {
            "Gally looks at " + player.getName() + " suspiciously and points them toward the camp grounds.",
            "Gally: 'Go head over that way - Newt will show you around.'"
        };
        return;
    }

    if (toLower(n) == "alby") {
        out.lines = {
            "Alby: 'So you are the Greenie of the month. Do you remember how you got here? Your name?'",
            "Alby: 'I see you are confused. Dont worry youll get your name back in a couple days. Its the one thing they let us keep.'"
        };
        out.options = { "A. Ask him 'what is this place?'", "B. Stay quiet and let him explain" };
        return;
    }

    if (toLower(n) == "newt") {
        int dlg = getDialogueProgress();
        int lvl = player.getLevel();

        if (dlg == 0 && lvl == 0) {
            out.lines = { "Newt: 'Hiya! You must be the new guy! I'm Newt, welcome to the Glade.'" };
            out.options = { "A. Yeah.. what is this place?", "B. It is nice to meet you - what is with these giant walls?" };
            return;
        }
        if (dlg == 1 && lvl == 0) {
            out.lines = { "Newt: 'Hello friend! What can I do you for?'" };
            out.options = { "A. I want to know more about the runners", "B. What kind of monsters were you saying live in the maze?" };
            return;
        }
        if (lvl == 1) {
            std::string roomName = player.getLocation() ? player.getLocation()->getName() : "";
            if (roomName == "The Shed" && dlg == 2) {
                out.lines = { "Newt: 'Hey friend! You still want to be a runner? Let's make you some gear! You'll need to find materials around the glade - metal, rocks, and sticks work best.'" };
                setDialogueProgress(3);
                return;
            }
            if (roomName == "The Shed" && dlg == 3) {
                out.lines = { "Newt: 'Did you get those materials?'" };
                out.options = { "A. Yes", "B. No" };
                return;
            }
            if (roomName == "The Shed" && dlg >= 4) {
                out.lines = { "Newt: 'The maze should be in its first cycle - if you go now, you can probably make it to the maze's edge by nightfall.'" };
                return;
            }
        }
        if (lvl > 1) {
            bool hasMagic = player.hasMagic();
            out.lines = { hasMagic
                ? "Newt: 'You survived the maze! Did you learn anything? Why do you look so ragged - do you need to see the medic?'"
                : "Newt: 'You survived the maze! Did you learn anything? You look ragged - go see the medic.'" };
            if (hasMagic) {
                out.options = { "A. Tell Newt about the Griever you killed and what you took from it", "B. Only tell him about the Griever you killed" };
            }
            return;
        }
        out.lines = { "Newt: 'Hello friend!'" };
        return;
    }

    if (toLower(n) == "minho") {
        out.lines = { "Minho: 'Ready to train?'" };
        out.options = { "A. Yes, I want to train", "B. No, not right now" };
        return;
    }

    if (toLower(n) == "terrisa") {
        out.lines = { "Terrisa: 'I... I know you. Your name is Thomas, isn't it?'" };
        out.options = { "A. How do you know my name?", "B. Are you alright? What happened to you?" };
        return;
    }

    out.lines = { getName() + " has nothing to say." };
}

void Helper::continueTalk(Player& player, DialogueState& out, int choice) {
    std::string n = getName();
    out.active = true;
    out.speaker = n;
    out.npc = this;
    out.options.clear();

    if (toLower(n) == "alby") {
        int lvl = player.getLevel();
        (void)lvl;
        if (choice == 0) {
            out.lines = { "Alby: 'This place is called the Glade. Let me show you.' he says, offering a tour." };
        } else {
            out.lines = { "Alby: 'I think a tour of the glade will help you understand things better.'" };
        }
        out.clear_after = true;
        return;
    }

    if (toLower(n) == "newt") {
        int dlg = getDialogueProgress();
        int lvl = player.getLevel();
        int step = g_subStep[this];

        if (dlg == 0 && lvl == 0) {
            if (step == 0) {
                out.lines = { choice == 0
                    ? "Newt: 'You see the walls surrounding us? Well those make up a maze that moves every few hours. Me and some of the others here have been here for over 3 years, every month a new person is sent up the hatch you came out of earlier, along with supplies.'"
                    : "Newt: 'Oh you noticed the walls- Well those make up a maze that moves every few hours. Me and some of the others here have been here for over 3 years, every month a new person is sent up the hatch you came out of earlier, along with supplies.'" };
                out.options = { "A. So the glade is like a prison?", "B. Have you guys tried to explore the maze?", "C. Why do you stay here?" };
                g_subStep[this] = 1;
                return;
            }
            if (step == 1) {
                if (choice == 0) {
                    out.lines = { "Newt: 'Not a prison, a safe zone. There are monsters that hunt anyone who enters the maze.'" };
                    out.options = { "A. So the glade is like a prison?", "B. Have you guys tried to explore the maze?", "C. Why do you stay here?" };
                    return; // stay on step 1 until they pick B
                } else if (choice == 1) {
                    out.lines = { "Newt: 'We have a group of people called the runners, their job is to explore the maze and map it so we can escape,' he says, pointing over to a runner." };
                    setDialogueProgress(1);
                    out.clear_after = true;
                    return;
                } else {
                    out.lines = { "Newt: 'There are monsters that hunt anyone who enters the maze, their venom has driven many of us insane.'" };
                    out.options = { "A. So the glade is like a prison?", "B. Have you guys tried to explore the maze?", "C. Why do you stay here?" };
                    return;
                }
            }
        }
        if (dlg == 1 && lvl == 0) {
            if (step == 0) {
                if (choice == 0) {
                    out.lines = { "Newt: 'The runners go into the maze and map it while trying not to get caught by Grievers. I make their gear in my shop.'" };
                    out.options = { "A. Can I be a runner?", "B. What happens when you get hit by these monsters?" };
                    g_subStep[this] = 1;
                } else {
                    out.lines = { "Newt: 'We call them Grievers. No one has been attacked and lived to tell about it. We can't let any runner who gets stung back into the Glade.'" };
                    out.clear_after = true;
                }
                return;
            }
            if (step == 1) {
                if (choice == 0) {
                    out.lines = {
                        "Newt: 'I don't see why not - you'll need gear though. Come see me at the shed later and I'll show you around the crafting shop.' He points south.",
                        "Newt: 'If you want to know more you should go chat with Minho - he can help you train to be strong enough to fight the monsters.. or at least run away from them. You can find him over west from the cage you came out of.'"
                    };
                    setDialogueProgress(2);
                    player.setExp(100.0); // triggers level 1
                } else {
                    out.lines = { "Newt: 'I can only tell you that if you are a runner.' Newt laughs." };
                }
                out.clear_after = true;
                return;
            }
        }
        if (lvl == 1 && getDialogueProgress() == 3) {
            if (choice == 0) {
                out.lines = {
                    "Newt: 'Perfect! Here are the crafting rules:'",
                    "  Rock + Stick   = Spear",
                    "  Metal + Stick  = Sword",
                    "  Leather + Cloth = Leather Armor",
                    "Type: craft <item name>"
                };
                setDialogueProgress(4);
            } else {
                out.lines = { "Newt: 'Alrighty, come back when you do.'" };
            }
            out.clear_after = true;
            return;
        }
        if (lvl > 1) {
            out.lines = { choice == 0
                ? "Newt: 'What? That is crazy - I've never heard of someone killing a Griever before. Maybe go see if that vial is useful with the medic.'"
                : "Newt: 'What? That is crazy - I've never heard of someone killing a Griever before. Go see the medic and get checked out.'" };
            out.clear_after = true;
            return;
        }
    }

    if (toLower(n) == "minho") {
        int step = g_subStep[this];
        if (step == 0) {
            if (choice == 0) {
                out.lines = { "Minho: 'Great! Let's get started.'", "Minho: 'What would you like to train on?'" };
                out.options = { "A. Combat techniques", "B. Endurance", "C. Speed", "D. Strength" };
                g_subStep[this] = 1;
            } else {
                out.lines = { "Minho: 'Alright, come back when you're ready to train.'" };
                out.clear_after = true;
            }
            return;
        }
        if (step == 1) {
            CombatState combat;
            if (choice == 0) {
                out.lines = { "Minho: 'Good choice! Combat techniques are essential for survival in the maze. You'll be fighting me for your training.'" };
                player.setInCombat(true);
                player.setCombatTarget(this);
                player.threat(this, combat);
            } else if (choice == 1) {
                out.lines = { "Minho: 'Endurance is key to lasting long in the maze. You'll need it to survive the trials. We'll start with some light sparring to build your stamina. Come at me!'" };
                player.setInCombat(true);
                player.setCombatTarget(this);
                player.attack(this, combat);
            } else if (choice == 2) {
                out.lines = { "Minho: 'Speed will help you avoid danger and reach your destination quickly and stay alive in the maze. Try to hit me - no swords, just try to hit me.'" };
                player.setInCombat(true);
                player.setCombatTarget(this);
                player.attack(this, combat);
            } else {
                out.lines = { "Minho: 'Strength will make you more formidable in combat. You'll be fighting a training dummy for training.'" };
            }
            out.clear_after = true;
            return;
        }
    }

    if (toLower(n) == "terrisa") {
        out.lines = { choice == 0
            ? "Terrisa: 'I don't know. It's just... there, in my head. Like the maze put it there.'"
            : "Terrisa: 'I'll be fine. But the maze - it's not moving anymore. Something changed when I came out.'" };
        out.clear_after = true;
        return;
    }

    out.clear_after = true;
}

// ---------------------------------------------------------- Monster ----
Monster::Monster(const std::string& name, const std::string& description,
                 double health, const Stats& stats, double exp)
    : NPC(name, description, health, stats) {
    energy = 10;
    experience = exp;
}
void Monster::update() {}

// ------------------------------------------------------------- Medic ----
Medic::Medic(const std::string& name, const std::string& description,
             double health, const Stats& stats)
    : NPC(name, description, health, stats) {}
void Medic::update() {}

void Medic::talk(Player& player, DialogueState& out) {
    out.active = true;
    out.speaker = getName();
    out.npc = this;
    out.options.clear();
    g_subStep[this] = 0;

    if (toLower(getName()) == "pete") {
        bool hasMagic = player.hasMagic();
        bool hasJuice = player.hasItem("Monster juice");
        out.lines = { "Pete: 'Hello, welcome to the infirmary, how can I help you?'" };
        if (hasJuice || hasMagic) {
            out.options = { "A. I went into the maze and got hurt", "B. Can you take a look at what I took from the Griever I killed?" };
        } else {
            out.options = { "A. I went into the maze and got hurt", "B. Do you get a lot of runners in here?" };
        }
        return;
    }
    out.lines = { getName() + " has nothing to say." };
}

void Medic::continueTalk(Player& player, DialogueState& out, int choice) {
    out.active = true;
    out.speaker = getName();
    out.npc = this;
    out.options.clear();

    if (toLower(getName()) == "pete") {
        bool hasMagic = player.hasMagic();
        bool hasJuice = player.hasItem("Monster juice");
        int step = g_subStep[this];

        if (step == 0) {
            if (choice == 0) {
                out.lines = { "Pete: 'Feel free to wait here and rest - you'll feel better in no time.'" };
                player.setHealth(player.MaxHealth());
                out.clear_after = true;
                return;
            }
            if (hasJuice || hasMagic) {
                out.lines = {
                    "Pete: 'This looks like a concentrated form of the Griever's venom. I could probably synthesize this into a serum to give us our memories back. Come back in a few minutes and you can try it.'",
                    "Pete: 'It's done! Would you like to test it?'"
                };
                out.options = { "A. Sure - what do I have to lose", "B. No, I'm not so sure about this" };
                g_subStep[this] = 1;
            } else {
                out.lines = { "Pete: 'More than you might think. You should take a cot and get some rest.'" };
                out.clear_after = true;
            }
            return;
        }
        if (step == 1) {
            if (choice == 0) {
                out.lines = {
                    "The serum brings back your memories of your family and life before the Maze, as well as your name.",
                    "You also feel an energy surging through you as if it is about to explode out of you.",
                    "[You can now use 'emp' to release an electromagnetic pulse. Watch your energy stat.]"
                };
                player.setMagic(true);
                player.removeItem("Monster juice");
            } else {
                out.lines = { "Pete: 'Ok, I hope you feel better and can continue to be a runner.'" };
            }
            out.clear_after = true;
            return;
        }
    }
    out.clear_after = true;
}