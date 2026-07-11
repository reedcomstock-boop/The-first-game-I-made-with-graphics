#include "GameLoop.h"
#include "updater.h"
#include "world.h"
#include <iostream>
#include <sstream>
#include <algorithm>

GameLoop::GameLoop(World& world, Player& player)
    : world(world), player(player), playing(true) {}


static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
void GameLoop::runFrame(const std::string& input) {
    (void)input;
    // Implementation for running a single frame
}
bool GameLoop::isPlaying() const {
    return playing;
}
/*void GameLoop::run() {
    printSituation();

    while (playing) {
        checkWorldProgression();

        std::cout << "\nWhat now? ";
        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string verb;
        ss >> verb;
        verb = toLower(verb);

        std::string arg;
        std::getline(ss, arg);
        if (!arg.empty() && arg[0] == ' ') arg = arg.substr(1);

        bool success = true;

        if      (verb == "go")        success = cmdGo(toLower(arg));
        else if (verb == "pickup")    success = cmdPickup(arg);
        else if (verb == "drop")      success = cmdDrop(arg);
        else if (verb == "inventory") success = cmdInventory();
        else if (verb == "equip")     success = cmdEquip(arg);
        else if (verb == "unequip")   success = cmdUnequip(arg);
        else if (verb == "attack")    success = cmdAttack(arg);
        else if (verb == "look")      { cmdLook(); }
        else if (verb == "magic")     success = cmdUseMagic();
        else if (verb == "talk")      success = cmdTalk(arg);
        else if (verb == "use")       success = cmdUseTool(arg);
        else if (verb == "flee")      { cmdFlee(); }
        else if (verb == "me")        { cmdMe(); }
        else if (verb == "help")      { showHelp(); }
        else if (verb == "exit")      { playing = false; }
        else {
            std::cout << "Not a valid command. Type 'help' for a list.\n";
            success = false;
        }

        if (success && verb == "go") printSituation();
    }

    std::cout << "\nYour strength finally gives out.\n"
              << "With no path left to take, you fall to the Maze floor.\n"
              << "Your journey ends here — unfinished, but not forgotten.\n";
}*/
void GameLoop::printSituation() const {
    Room* loc = player.getLocation();
    if (!loc) return;

    std::cout << "\n=== " << loc->getName() << " ===\n";
    std::cout << loc->getDescription() << "\n\n";

    if (!loc->getNpcEntities().empty()) {
        std::cout << "Characters here:\n";
        for (const auto& n : loc->getNpcEntities())
            std::cout << "  " << n->getName() << "\n";
        std::cout << "\n";
    }

    if (!loc->getItems().empty()) {
        std::cout << "Items on the ground:\n";
        for (Item* i : loc->getItems())
            std::cout << "> " << i->getName() << "\n";
        std::cout << "\n";
    }

    std::cout << "Exits: \n";
    for (const auto& e : loc->getExits())
    std::cout << "> " << e.first << " {" << e.second->getName() << "} \n";
}

void GameLoop::showHelp() const {
    std::cout << "\n--- Commands ---\n"
              << "go <direction>       -- move (north / south / east / west / up / down)\n"
              << "look                 -- describe the current room\n"
              << "pickup <item>        -- pick up an item\n"
              << "drop <item>          -- drop an item from your inventory\n"
              << "inventory            -- list everything you're carrying\n"
              << "use <Tool>           -- use a tool in your inventory\n"
              << "equip <item>         -- equip a tool to gain its stat bonuses\n"
              << "unequip <item>       -- remove a tool's bonuses\n"
              << "talk <npc>           -- talk to an NPC\n"
              << "attack <monster>     -- fight a monster\n"
              << "flee                 -- attempt to flee from combat\n"
              << "magic                -- use magic against a monster (if you have it)\n"
              << "me                   -- check your current stats and objective\n"
              << "help                 -- show this list\n"
              << "exit                 -- quit the game\n\n";
}
bool GameLoop::cmdGo(const std::string& direction) {
    Room* nextRoom = player.goDirection(direction);
    if (!nextRoom) {
        std::cout << "You can't go that way.\n";
        return false;
    }
    Room* current = player.getLocation();
    if (current->getName() == "The Walls" && direction == "north") {
        if (world.getWorldLevel() < 3) {
            std::cout << "\nGally steps in front of you.\n";
            //if 
            std::cout << "Gally: 'Where do you think you're going Greenie? You got a Death Wish or something? Only Runners are allowed to go through the Maze. You need to prove yourself first.'\n";
            std::cout << "You're not a runner yet. Go talk to Newt.'\n";
            return false;
        }
    }

    // Check if any monsters are in the new room
    if (!nextRoom->getMonsterEntities().empty()) {
        for (const auto& monster : nextRoom->getMonsterEntities()) {

            // Roll 0-9, threat triggers if roll >= (dex + intel) / 2
            double avoidChance = (player.getStats().dexterity + player.getStats().intelligence) / 2.0;
            int roll = rand() % 10;

            if (roll >= avoidChance) {
                std::cout << monster->getName() << " notices you entering the room!\n";
                player.threat(monster);
                std::cout << "Choose: attack, flee, or magic.\n";
            }
            else {
                std::cout << "You slip past " << monster->getName() << " unnoticed.\n";
            }
        }
    }

    return true;
}
bool GameLoop::cmdLook()const{
    //Room* currentRoom = player.getLocation();
    std::cout << "you look around and see ";
    printSituation();
    return true;
    }
bool GameLoop::cmdPickup(const std::string& itemName) {
    Room* loc = player.getLocation();
    if (!loc) return false;

    for (Item* item : loc->getItems()) {
        if (toLower(item->getName()) == toLower(itemName)) {
            player.PickUpItem(item);
            loc->removeItem(item);
            std::cout << "Picked up '" << item->getName() << "'.\n";
            return true;
        }
    }
    std::cout << "No item called '" << itemName << "' here.\n";
    return false;
}
bool GameLoop::cmdDrop(const std::string& itemName){
    Room* loc = player.getLocation();

    for (Item* item : player.getItems()) {
        if (toLower(item->getName()) == toLower(itemName)) {
            player.DropItem(item);
            loc->addItem(item);
            std::cout << "Dropped: '" << item->getName() << "'. If you want it back come back here to pick it up.\n";
            Tool* tool = dynamic_cast<Tool*>(item);
            if (tool) {
                player.unequipItem(tool);
            }
            return true;
        }
    }
    std::cout << "No item called '" << itemName << "' here.\n";
    return false;
}
bool GameLoop::cmdUseTool(const std::string& itemName){
    for (Item* item: player.getInventory()) {
        if (toLower(item->getName()) == toLower(itemName)) {
            std::cout << "You use the " << item->getName() << ".\n";
            player.useTool(item->getName());
            return true;
        }
    }
    std::cout << "No tool called '" << itemName << "' in your inventory.\n";
    return false;
}
bool GameLoop::cmdInventory() const {
    const auto& items = player.getItems();
    if (items.empty()) {
        std::cout << "Your inventory is empty.\n";
        return true;
    }
    std::cout << "Inventory:\n";
    for (Item* item : items) {
        Tool* t = dynamic_cast<Tool*>(item);
        if (t)
            std::cout << "  [TOOL] " << item->getName() << "\n";
        else
            std::cout << "  " << item->getName() << "\n";
    }
    return true;
}

bool GameLoop::cmdEquip(const std::string& itemName){
    for (Item* item : player.getItems()){
         if (toLower(item->getName()) == toLower(itemName)) {
            Tool* tool = dynamic_cast<Tool*>(item);
            if (!tool) {
                std::cout << tool->getName() << " is not a tool.\n";
                return false;
            }
            player.equipItem(tool);
            std::cout << "Equipped " << tool->getName() << ".\n";
            return true;
        }
    }
    std::cout << "You don't have '" << itemName << "'.\n";
    return false;
}

bool GameLoop::cmdUnequip(const std::string& itemName){
    for (Item* item : player.getItems()) {
        if (toLower(item->getName()) == toLower(itemName)) {
           Tool* tool = dynamic_cast<Tool*>(item);
            if (tool) {
                player.unequipItem(tool);
                std::cout << "Unequipped '" << tool->getName() << "'.\n";
                return true;
            }
        }
    }
    std::cout << "'" << itemName << "' is not equipped.\n";
    return false;
}
bool GameLoop::cmdAttack(const std::string& targetName) {
    // If already in combat, "attack" just means "attack again this round"
    if (player.getInCombat()) {
        player.attack(player.getCombatTarget());
        return true;
    }

    Room* loc = player.getLocation();
    if (!loc) return false;

    for (const auto& npcEntity : loc->getNpcEntities()) {
        if (toLower(npcEntity->getName()) == toLower(targetName)) {
            NPC* target = world.getNpcByName(npcEntity->getName());
            if (target) {
                player.setInCombat(true);
                player.setCombatTarget(target);
                std::cout << "You engage " << target->getName() << " in combat!\n";
                player.attack(target); // first round
            } else {
                std::cout << "you cant fight '" << targetName << "'\n";
            }
            return true;
        }
    }
    std::cout << "No monster called '" << targetName << "' here.\n";
    return false;
}
bool GameLoop::cmdFlee() {
    player.fleeComnbat();
    return true;
}
bool GameLoop::cmdTalk(const std::string& npcName) {
    Room* loc = player.getLocation();
    if (!loc) return false;

    NPC* target = loc->getNpcByName(npcName);
    if (!target) {
        std::cout << "There is no one called '" << npcName << "' here.\n";
        return false;
    }

    target->talk(player);
    std::cout << "\nPress enter to continue...";
    std::string pause;
    std::getline(std::cin, pause);
    printSituation();
    return true;
}
bool GameLoop::cmdUseMagic() {
    if (!player.getInCombat()) {
        std::cout << "There's nothing to cast magic at.\n";
        return false;
    }
    player.useMagic();
    return true;
}
bool GameLoop::cmdMe() const {
    std::cout << "\n--- Status ---\n";
    std::cout << "Experience To Level Up: " << player.getExp() << " out of " << player.getExpToNextLevel() << "\n";
    std::cout << "Name:          " << player.getName() << "\n";
    std::cout << "Room:          " << player.getLocation()->getName() << "\n";
    std::cout << "Health:        " << player.getHealth()<< " out of "<< player.MaxHealth()<<" \n";
    std::cout << "Energy:        " << player.getEnergy() << " out of "<< player.MaxEnergy()<<" \n";
    std::cout << "Level:         " << player.getLevel() << "\n";
    std::cout << "Strength:      " << player.getStats().strength << "\n";
    std::cout << "Dexterity:     " << player.getStats().dexterity << "\n";
    std::cout << "Intelligence:  " << player.getStats().intelligence << "\n";
    std::cout << "Defence:       " << player.getStats().defence << "\n";
   if (player.getLevel() == 2){
        std::cout << "Magic:          " << player.hasMagic() << "\n";
    }

    std::cout << "Equipped:\n";
    if (player.getInventory().empty()) {
        std::cout << "> nothing\n";
    }
    else {
        for (Item* item : player.getInventory())
            std::cout << "> " << item->getName() << "\n";
    }

    if (player.getLevel() == 0){
        std::cout << "\nObjective: Talk to Newt until he tells you about being a runner.\n";
    }
    else if (player.getLevel() == 1){
        std::cout << "\nObjective: Craft your gear and run through the first cycle of the maze.\n";
    }
    else if (player.getLevel() == 2){
        std::cout << "\nObjective: Find your way back to camp and report to the infirmary and to Newt.\n";
    }
    else if (player.getLevel() == 3){
        std::cout << "\nObjective: Find a way to beat the monsters and find the way out. (Hint: EMP?)\n";
}
    return true;
}
void GameLoop::checkWorldProgression() {
    int wl = world.getWorldLevel();
    int pl = player.getLevel();
    Room* loc = player.getLocation();
    if (!loc) return;

    // Player has explored enough — evolve the world
    if (pl >= 1 && wl == 1) {
        world.setWorldLevel(2);
        world.createWorldLvlTwo();
        std::cout << "\n[The world shifts around you — things are changing...]\n";
    }

    // Player enters the maze for the first time
    if (loc->getName() == "The Maze" && wl == 2) {
        world.setWorldLevel(3);
        world.createMaze();
        std::cout << "\n[The maze opens before you...]\n";
    }

    // Player reaches level 2 — maze gets harder
    if (pl >= 2 && wl == 3) {
        world.setWorldLevel(4);
        world.createMazePhaseTwo();
        std::cout << "\n[The maze reconfigures itself around you...]\n";
    }
}

