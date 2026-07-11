#include "Entity.h"
#include "player.h"
#include "rooms.h"
#include <iostream>
#include <sstream>
#include <algorithm>

std::string Entity::toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
// Entity constructor - matches the header exactly, no extra params
Entity::Entity(const std::string& name, const std::string& description,
               double health, const Stats& stats)
    : name(name), description(description), health(health), stats(stats) {
    energy = 0;
    experience = 0;
    level = 1;
    dialogueProgress = 0;   
}
void Entity::update() {
    energy -= 1; // Decrease energy over time
    if (energy <= 1) {
        setHealth(getHealth() - 1); // Decrease health if energy is too low 
    }
}
std::string Entity::getName() const { 
    return name; 
}
std::string Entity::getDescription() const { 
    return description; 
}
double Entity::getHealth() const { 
    return health; 
}
const Stats& Entity::getStats() const { 
    return stats; 
}
void Entity::setName(const std::string& n) { 
    this->name = n; 
}
void Entity::setDescription(const std::string& d) { 
    this->description = d; 
}
void Entity::setHealth(double h) { 
    this->health = h; 
}
double Entity::getEnergy() const {
    return energy;
}
void Entity::setEnergy(double e) {
    this->energy = e;
}
void Entity::applyBonus(const Stats& bonus) { 
    stats += bonus; 
}
double Entity::getExperience() const { 
    return experience; 
}
int32_t Entity::getDialogueProgress() const { 
    return dialogueProgress; 
}
void Entity::setDialogueProgress(int32_t progress) { 
    dialogueProgress = progress; 
}

// NPC
NPC::NPC(const std::string& name, const std::string& description,
         double health, const Stats& stats)
    : Entity(name, description, health, stats) {}

    
void NPC::update() {
    energy -= 1;
    if (energy <= 1) {
        setHealth(getHealth() - 1);
    }
}
void NPC::talk(Player& ) {
    std::cout << getName() << " has nothing to say.\n";
}

// Helper
Helper::Helper(const std::string& name, const std::string& description,
               double health, const Stats& stats)
    : NPC(name, description, health, stats) {}

void Helper::talk(Player&player) {
    Room* loc = player.getLocation();
    std::string n = getName();

    if (toLower(n) == "gally") {
        std::cout << "\nGally looks at " << player.getName() << " suspiciously and points them toward the camp grounds.\n";
        std::cout << "Gally: 'Go head over that way — Newt will show you around.'\n";
    }
    else if (toLower(n) == "newt") {
        int dlg = getDialogueProgress();
        int lvl = player.getLevel();
        std::string choice; 
        if (dlg == 0 && lvl == 0) {
            std::cout << "\nNewt: 'Hiya! You must be the new guy! I'm Newt, welcome to the Glade.'\n";
            std::cout << "A. Yeah.. what is this place?\n";
            std::cout << "B. It is nice to meet you — what is with these giant walls?\n: ";
            std::getline(std::cin, choice);
            std::cout << "\nNewt: 'You see the walls surrounding us? We are trapped in the middle of a maze full of monsters and those walls keep them away from us.'\n";
            setDialogueProgress(1);
        } 
        else if (dlg == 1 && lvl == 0) {
            std::cout << "\nNewt: 'Hello friend! What can I do you for?'\n";
            std::cout << "A. I want to know more about the runners\n";
            std::cout << "B. What kind of monsters were you saying live in the maze?\n: ";
            std::getline(std::cin, choice);
            if (toLower(choice) == "a") {
                std::cout << "\nNewt: 'The runners go into the maze and map it while trying not to get caught by Grievers. I make their gear in my shop.'\n";
                std::cout << "A. Can I be a runner?\n";
                std::cout << "B. What happens when you get hit with the venom?\n: ";
                std::getline(std::cin, choice);
                if (toLower(choice) == "a") {
                    std::cout << "\nNewt: 'I don't see why not — you'll need gear though. Come see me at the shed and I'll show you around the crafting shop.' He points south.\n";
                    setDialogueProgress(2);
                    player.setExp(100.0); // triggers level 1
                    //printSituation();
                } 
                else {
                    std::cout << "\nNewt: 'I can only tell you that if you are a runner.' Newt laughs.\n";
                }
            } 
            else {
                std::cout << "\nNewt: 'We call them Grievers. No one has been attacked and lived to tell about it. We can't let any runner who gets stung back into the Glade.'\n";
            }

        } 
        else if (lvl == 1) {
            std::string roomName = loc->getName();
            if (roomName == "The Shed" && dlg == 2) {
                std::cout << "\nNewt: 'Hey friend! You still want to be a runner? Let's make you some gear! You'll need to find materials around the glade — metal, rocks, and sticks work best.'\n";
                setDialogueProgress(3);
            } 
            else if (roomName == "The Shed" && dlg == 3) {
                std::cout << "\nNewt: 'Did you get those materials?'\n";
                std::cout << "A. Yes\nB. No\n: ";
                std::getline(std::cin, choice);
                if (toLower(choice) == "a") {
                    std::cout << "\nNewt: 'Perfect! Here are the crafting rules:'\n";
                    std::cout << "  Rock + Stick   = Spear\n";
                    std::cout << "  Metal + Stick  = Sword\n";
                    std::cout << "  Leather + Cloth = Leather Armor\n";
                    std::cout << "Type: craft <item name>\n";
                    setDialogueProgress(4);
                } 
                else {
                    std::cout << "\nNewt: 'Alrighty, come back when you do.'\n";
                }
            } 
            else if (roomName == "The Shed" && dlg >= 4) {
                std::cout << "\nNewt: 'The maze should be in its first cycle — if you go now, you can probably make it to the maze's edge by nightfall.'\n";
            }

        } 
        else if (lvl > 1) {
            bool hasMagic = player.hasMagic();
            if (hasMagic) {
                std::cout << "\nNewt: 'You survived the maze! Did you learn anything? Why do you look so ragged — do you need to see the medic?'\n";
                std::cout << "A. Tell Newt about the Griever you killed and what you took from it\n";
                std::cout << "B. Only tell him about the Griever you killed\n: ";
                std::getline(std::cin, choice);
                if (toLower(choice) == "a") {
                    std::cout << "\nNewt: 'What? That is crazy — I've never heard of someone killing a Griever before. Maybe go see if that vial is useful with the medic.'\n";
                } 
                else {
                    std::cout << "\nNewt: 'What? That is crazy — I've never heard of someone killing a Griever before. Go see the medic and get checked out.'\n";
                }
            } 
            else {
                std::cout << "\nNewt: 'You survived the maze! Did you learn anything? You look ragged — go see the medic.'\n";
            }
        }
    }
    
    else if (toLower(n) == "minho") {
        std::cout << "\nMinho: 'Ready to train?'\n";
    }
    else {
        std::cout << getName() << " has nothing to say.\n";
    }
}

void Helper::update() {
    NPC::update();  // reuse NPC's update logic
}

// Monster - no extra params, energy starts at 10
Monster::Monster(const std::string& name, const std::string& description,
                 double health, const Stats& stats, double exp)
    : NPC(name, description, health, stats) {
    energy = 10;
    experience = exp;
}

void Monster::update() {
    // monsters move randomly - movement logic will go here
    // when World is accessible from here
}

// Medic
Medic::Medic(const std::string& name, const std::string& description,
             double health, const Stats& stats)
    : NPC(name, description, health, stats) {}

void Medic::update() {
    // medics don't do anything on update for now
}
void Medic::talk(Player& player) {
    if (toLower(getName()) == "pete") {
        bool hasMagic = player.hasMagic();
        bool hasJuice = player.hasItem("Monster juice");
        std::string choice;  

        if (hasJuice || hasMagic) {
            std::cout << "\nPete: 'Hello, welcome to the infirmary, how can I help you?'\n";
            std::cout << "A. I went into the maze and got hurt\n";
            std::cout << "B. Can you take a look at what I took from the Griever I killed?\n: ";
            std::getline(std::cin, choice);
            if (toLower(choice) == "a") {
                std::cout << "\nPete: 'Feel free to wait here and rest — you'll feel better in no time.'\n";
                player.setHealth(player.MaxHealth());
            } 
            else {
                std::cout << "\nPete: 'This looks like a concentrated form of the Griever's venom. I could probably synthesize this into a serum to give us our memories back. Come back in a few minutes and you can try it.'\n";
                std::cout << "\nPete: 'It's done! Would you like to test it?'\n";
                std::cout << "A. Sure — what do I have to lose\n";
                std::cout << "B. No, I'm not so sure about this\n: ";
                std::getline(std::cin, choice);
                if (toLower(choice) == "a") {
                    std::cout << "\nThe serum brings back your memories of your family and life before the Maze, as well as your name.\n";
                    std::cout << "You also feel an energy surging through you as if it is about to explode out of you.\n";
                    std::cout << "[You can now use 'emp' to release an electromagnetic pulse. Watch your energy stat.]\n";
                    player.setMagic(true);
                    player.removeItem("Monster juice");
                } 
                else {
                    std::cout << "\nPete: 'Ok, I hope you feel better and can continue to be a runner.'\n";
                }
            }
        } 
        else {
            std::cout << "\nPete: 'Hello, welcome to the infirmary, how can I help you?'\n";
            std::cout << "A. I went into the maze and got hurt\n";
            std::cout << "B. Do you get a lot of runners in here?\n: ";
            std::getline(std::cin, choice);
            if (toLower(choice) == "a") {
                std::cout << "\nPete: 'Feel free to wait here and rest — you'll feel better in no time.'\n";
                player.setHealth(player.MaxHealth());
            } 
            else {
                std::cout << "\nPete: 'More than you might think. You should take a cot and get some rest.'\n";
            }
        }
    }
}
