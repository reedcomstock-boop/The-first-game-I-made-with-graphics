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
// Keyed by NPC pointer so Brecht/Rooke/Wren/Isolde don't share state.
#include <unordered_map>
static std::unordered_map<NPC*, int> g_subStep;

void Helper::talk(Player& player, DialogueState& out) {
    std::string n = getName();
    out.active = true;
    out.speaker = n;
    out.npc = this;
    out.options.clear();
    g_subStep[this] = 0;  // fresh conversation entry point

    if (toLower(n) == "brecht") {
        out.lines = {
            "Brecht looks at " + player.getName() + " suspiciously and points them toward the camp grounds.",
            "Brecht: 'Go head over that way - Aldric will show you around.'"
        };
        return;
    }

    if (toLower(n) == "isolde") {
        out.lines = {
            "Isolde: 'So you are the Greenie of the month. Do you remember how you got here? Your name?'",
            "Isolde: 'I see you are confused. Dont worry youll get your name back in a couple days. Its the one thing they let us keep.'"
        };
        out.options = { "A. Ask her 'what is this place?'", "B. Stay quiet and let her explain" };
        return;
    }

    if (toLower(n) == "aldric") {
        int dlg = getDialogueProgress();
        int lvl = player.getLevel();

        if (dlg == 0 && lvl == 0) {
            out.lines = { "Aldric: 'Hiya! You must be the new guy! I'm Aldric, welcome to the Hollow.'" };
            out.options = { "A. Yeah.. what is this place?", "B. It is nice to meet you - what is with these giant walls?" };
            return;
        }
        if (dlg == 1 && lvl == 0) {
            out.lines = { "Aldric: 'Hello friend! What can I do you for?'" };
            out.options = { "A. I want to know more about the Pathfinders", "B. What kind of monsters were you saying live in the labyrinth?" };
            return;
        }
        if (lvl == 1) {
            std::string roomName = player.getLocation() ? player.getLocation()->getName() : "";
            if (roomName == "The Shed" && dlg == 2) {
                out.lines = { "Aldric: 'Hey friend! You still want to be a Pathfinder? Let's make you some gear! You'll need to find materials around the Hollow - metal, rocks, and sticks work best.'" };
                setDialogueProgress(3);
                return;
            }
            if (roomName == "The Shed" && dlg == 3) {
                out.lines = { "Aldric: 'Did you get those materials?'" };
                out.options = { "A. Yes", "B. No" };
                return;
            }
            if (roomName == "The Shed" && dlg >= 4) {
                out.lines = { "Aldric: 'The maze should be in its first cycle - if you go now, you can probably make it to the labyrinth's edge by nightfall.'" };
                return;
            }
        }
        if (lvl > 1) {
            bool hasMagic = player.hasMagic();
            out.lines = { hasMagic
                ? "Aldric: 'You survived the labyrinth! Did you learn anything? Why do you look so ragged - do you need to see the medic?'"
                : "Aldric: 'You survived the labyrinth! Did you learn anything? You look ragged - go see the medic.'" };
            if (hasMagic) {
                out.options = { "A. Tell Aldric about the Warden you killed and what you took from it", "B. Only tell him about the Warden you killed" };
            }
            return;
        }
        out.lines = { "Aldric: 'Hello friend!'" };
        return;
    }

    if (toLower(n) == "rooke") {
        out.lines = { "Rooke: 'Ready to train?'" };
        out.options = { "A. Yes, I want to train", "B. No, not right now" };
        return;
    }

    if (toLower(n) == "wren") {
        out.lines = { "Wren: 'I... I know you. Your name is Callum, isn't it?'" };
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

    if (toLower(n) == "isolde") {
        int lvl = player.getLevel();
        (void)lvl;
        if (choice == 0) {
            out.lines = { "Isolde: 'This place is called the Hollow. Let me show you.' she says, offering a tour." };
        } else {
            out.lines = { "Isolde: 'I think a tour of the Hollow will help you understand things better.'" };
        }
        out.clear_after = true;
        return;
    }

    if (toLower(n) == "aldric") {
        int dlg = getDialogueProgress();
        int lvl = player.getLevel();
        int step = g_subStep[this];

        if (dlg == 0 && lvl == 0) {
            if (step == 0) {
                out.lines = { choice == 0
                    ? "Aldric: 'You see the walls surrounding us? Well those make up a maze that moves every few hours. Me and some of the others here have been here for over 3 years, every month a new person is sent up the hatch you came out of earlier, along with supplies.'"
                    : "Aldric: 'Oh you noticed the walls- Well those make up a maze that moves every few hours. Me and some of the others here have been here for over 3 years, every month a new person is sent up the hatch you came out of earlier, along with supplies.'" };
                out.options = { "A. So the Hollow is like a prison?", "B. Have you guys tried to explore the labyrinth?", "C. Why do you stay here?" };
                g_subStep[this] = 1;
                return;
            }
            if (step == 1) {
                if (choice == 0) {
                    out.lines = { "Aldric: 'Not a prison, a safe zone. There are monsters that hunt anyone who enters the labyrinth.'" };
                    out.options = { "A. So the Hollow is like a prison?", "B. Have you guys tried to explore the labyrinth?", "C. Why do you stay here?" };
                    return; // stay on step 1 until they pick B
                } else if (choice == 1) {
                    out.lines = { "Aldric: 'We have a group of people called the Pathfinders, their job is to explore the labyrinth and map it so we can escape,' he says, pointing over to a Pathfinder." };
                    setDialogueProgress(1);
                    out.clear_after = true;
                    return;
                } else {
                    out.lines = { "Aldric: 'There are monsters that hunt anyone who enters the labyrinth, their venom has driven many of us insane.'" };
                    out.options = { "A. So the Hollow is like a prison?", "B. Have you guys tried to explore the labyrinth?", "C. Why do you stay here?" };
                    return;
                }
            }
        }
        if (dlg == 1 && lvl == 0) {
            if (step == 0) {
                if (choice == 0) {
                    out.lines = { "Aldric: 'The Pathfinders go into the labyrinth and map it while trying not to get caught by Wardens. I make their gear in my shop.'" };
                    out.options = { "A. Can I be a Pathfinder?", "B. What happens when you get hit by these monsters?" };
                    g_subStep[this] = 1;
                } else {
                    out.lines = { "Aldric: 'We call them Wardens. No one has been attacked and lived to tell about it. We can't let any Pathfinder who gets stung back into the Hollow.'" };
                    out.clear_after = true;
                }
                return;
            }
            if (step == 1) {
                if (choice == 0) {
                    out.lines = {
                        "Aldric: 'I don't see why not - you'll need gear though. Come see me at the shed later and I'll show you around the crafting shop.' He points south.",
                        "Aldric: 'If you want to know more you should go chat with Rooke - he can help you train to be strong enough to fight the monsters.. or at least run away from them. You can find him over west from the cage you came out of.'"
                    };
                    setDialogueProgress(2);
                    player.setExp(100.0); // triggers level 1
                } else {
                    out.lines = { "Aldric: 'I can only tell you that if you are a Pathfinder.' Aldric laughs." };
                }
                out.clear_after = true;
                return;
            }
        }
        if (lvl == 1 && getDialogueProgress() == 3) {
            if (choice == 0) {
                out.lines = {
                    "Aldric: 'Perfect! Here are the crafting rules:'",
                    "  Rock + Stick   = Spear",
                    "  Metal + Stick  = Sword",
                    "  Leather + Cloth = Leather Armor",
                    "Type: craft <item name>"
                };
                setDialogueProgress(4);
            } else {
                out.lines = { "Aldric: 'Alrighty, come back when you do.'" };
            }
            out.clear_after = true;
            return;
        }
        if (lvl > 1) {
            out.lines = { choice == 0
                ? "Aldric: 'What? That is crazy - I've never heard of someone killing a Warden before. Maybe go see if that vial is useful with the medic.'"
                : "Aldric: 'What? That is crazy - I've never heard of someone killing a Warden before. Go see the medic and get checked out.'" };
            out.clear_after = true;
            return;
        }
    }

    if (toLower(n) == "rooke") {
        int step = g_subStep[this];
        if (step == 0) {
            if (choice == 0) {
                out.lines = { "Rooke: 'Great! Let's get started.'", "Rooke: 'What would you like to train on?'" };
                out.options = { "A. Combat techniques", "B. Endurance", "C. Speed", "D. Strength" };
                g_subStep[this] = 1;
            } else {
                out.lines = { "Rooke: 'Alright, come back when you're ready to train.'" };
                out.clear_after = true;
            }
            return;
        }
        if (step == 1) {
            CombatState combat;
            if (choice == 0) {
                out.lines = { "Rooke: 'Good choice! Combat techniques are essential for survival in the labyrinth. You'll be fighting me for your training.'" };
                player.setInCombat(true);
                player.setCombatTarget(this);
                player.threat(this, combat);
            } else if (choice == 1) {
                out.lines = { "Rooke: 'Endurance is key to lasting long in the labyrinth. You'll need it to survive the trials. We'll start with some light sparring to build your stamina. Come at me!'" };
                player.setInCombat(true);
                player.setCombatTarget(this);
                player.attack(this, combat);
            } else if (choice == 2) {
                out.lines = { "Rooke: 'Speed will help you avoid danger and reach your destination quickly and stay alive in the labyrinth. Try to hit me - no swords, just try to hit me.'" };
                player.setInCombat(true);
                player.setCombatTarget(this);
                player.attack(this, combat);
            } else {
                out.lines = { "Rooke: 'Strength will make you more formidable in combat. You'll be fighting a training dummy for training.'" };
            }
            out.clear_after = true;
            return;
        }
    }

    if (toLower(n) == "wren") {
        out.lines = { choice == 0
            ? "Wren: 'I don't know. It's just... there, in my head. Like the labyrinth put it there.'"
            : "Wren: 'I'll be fine. But the labyrinth - it's not moving anymore. Something changed when I came out.'" };
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

    if (toLower(getName()) == "doc marrow") {
        bool hasMagic = player.hasMagic();
        bool hasJuice = player.hasItem("Monster juice");
        out.lines = { "Doc Marrow: 'Hello, welcome to the infirmary, how can I help you?'" };
        if (hasJuice || hasMagic) {
            out.options = { "A. I went into the labyrinth and got hurt", "B. Can you take a look at what I took from the Warden I killed?" };
        } else {
            out.options = { "A. I went into the labyrinth and got hurt", "B. Do you get a lot of Pathfinders in here?" };
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

    if (toLower(getName()) == "doc marrow") {
        bool hasMagic = player.hasMagic();
        bool hasJuice = player.hasItem("Monster juice");
        int step = g_subStep[this];

        if (step == 0) {
            if (choice == 0) {
                out.lines = { "Doc Marrow: 'Feel free to wait here and rest - you'll feel better in no time.'" };
                player.setHealth(player.MaxHealth());
                out.clear_after = true;
                return;
            }
            if (hasJuice || hasMagic) {
                out.lines = {
                    "Doc Marrow: 'This looks like a concentrated form of the Warden's venom. I could probably synthesize this into a serum to give us our memories back. Come back in a few minutes and you can try it.'",
                    "Doc Marrow: 'It's done! Would you like to test it?'"
                };
                out.options = { "A. Sure - what do I have to lose", "B. No, I'm not so sure about this" };
                g_subStep[this] = 1;
            } else {
                out.lines = { "Doc Marrow: 'More than you might think. You should take a cot and get some rest.'" };
                out.clear_after = true;
            }
            return;
        }
        if (step == 1) {
            if (choice == 0) {
                out.lines = {
                    "The serum brings back your memories of your family and life before the labyrinth, as well as your name.",
                    "You also feel an energy surging through you as if it is about to explode out of you.",
                    "[You can now use 'emp' to release an electromagnetic pulse. Watch your energy stat.]"
                };
                player.setMagic(true);
                player.removeItem("Monster juice");
            } else {
                out.lines = { "Doc Marrow: 'Ok, I hope you feel better and can continue to be a Pathfinder.'" };
            }
            out.clear_after = true;
            return;
        }
    }
    out.clear_after = true;
}