#include "GameLoop.h"
#include "updater.h"
#include "Entity.h" 
#include "save.h"
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
    if (dialogue.active) {
        handleDialogueChoice(input);
        return;   // swallow input while a conversation is open
    }
    if (input.empty()) return;

    checkWorldProgression();

    std::istringstream ss(input);
    std::string verb;
    ss >> verb;
    verb = toLower(verb);

    std::string arg;
    std::getline(ss, arg);
    if (!arg.empty() && arg[0] == ' ') arg = arg.substr(1);

    if      (verb == "go")        cmdGo(toLower(arg));
    else if (verb == "pickup")    cmdPickup(toLower(arg));
    else if (verb == "drop")      cmdDrop(toLower(arg));
    else if (verb == "craft")     cmdCraft(toLower(arg));
    else if (verb == "inventory") cmdInventory();
    else if (verb == "equip")     cmdEquip(toLower(arg));
    else if (verb == "unequip")   cmdUnequip(toLower(arg));
    else if (verb == "attack")    cmdAttack(toLower(arg));
    else if (verb == "look")      cmdLook();
    else if (verb == "save")      cmdSave(arg.empty() ? "save.json" : arg);
    else if (verb == "load")      cmdLoad(arg.empty() ? "save.json" : arg);
    else if (verb == "magic")     cmdUseMagic();
    else if (verb == "talk")      cmdTalk(toLower(arg));
    else if (verb == "use")       cmdUseTool(toLower(arg));
    else if (verb == "flee")      cmdFlee();
    else if (verb == "me")        cmdMe();
    else if (verb == "help")      showHelp();
    else if (verb == "exit")      playing = false;
}
bool GameLoop::isPlaying() const {
    return playing;
}
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
              << "craft <item>         -- craft gear from materials (near Newt)\n"
              << "use <Tool>           -- use a tool in your inventory\n"
              << "equip <item>         -- equip a tool to gain its stat bonuses\n"
              << "unequip <item>       -- remove a tool's bonuses\n"
              << "talk <npc>           -- talk to an NPC\n"
              << "attack <monster>     -- fight a monster\n"
              << "flee                 -- attempt to flee from combat\n"
              << "magic                -- use magic against a monster (if you have it)\n"
              << "me                   -- check your current stats and objective\n"
              << "save <filename>      -- save your game (default: save.json)\n"
              << "load <filename>      -- load a saved game (default: save.json)\n"
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
        if (player.getLevel() < 2) {
            std::cout << "\nGally steps in front of you.\n";
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
                std::cout << item->getName() << " is not a tool.\n";
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
    if (!player.getLocation()) {
        std::cout << "You're nowhere — can't talk to anyone.\n";
        return false;
    }
    NPC* npc = player.getLocation()->getNpcByName(npcName);
    if (!npc) {
        std::cout << "There's no one here by that name.\n";
        return false;
    }
    npc->talk(player, dialogue);
    return true;
}
// New: called from runFrame() when the player types a single letter (a/b/c/d)
// while dialogue.active is true and dialogue has pending options.
bool GameLoop::handleDialogueChoice(const std::string& input) {
    if (!dialogue.active) return false;

    if (dialogue.options.empty()) {
        // No choice pending — any input just dismisses the current page
        if (dialogue.clear_after) dialogue.clear();
        return true;
    }

    std::string c = input;
    std::transform(c.begin(), c.end(), c.begin(), ::tolower);
    if (c.size() != 1 || c[0] < 'a') return false;
    int choiceIndex = c[0] - 'a';
    if (choiceIndex < 0 || choiceIndex >= (int)dialogue.options.size()) return false;

    if (dialogue.npc) {
        dialogue.npc->continueTalk(player, dialogue, choiceIndex);
    }
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
bool GameLoop::cmdCraft(const std::string& itemName) {
    Room* loc = player.getLocation();
    if (!loc) return false;

    if (!loc->getNpcByName("Newt")) {
        std::cout << "There's no one here to help you craft anything.\n";
        return false;
    }

    Tool* crafted = nullptr;

    if (itemName == "sword" && player.hasItem("metal") && player.hasItem("stick")) {
        crafted = player.craftItem("Sword",
            "a sword made of scrap metal and tied together with newts leather",
            1.0, {10, 15, 5, 5}, 0.0, 0.0);
        player.removeItem("metal");
        player.removeItem("stick");
    }
    else if (itemName == "spear" && player.hasItem("rock") && player.hasItem("stick")) {
        crafted = player.craftItem("Spear",
            "a spear made of a chunk of obsidian and a pole tied together with newts leather",
            1.0, {15, 10, 3, 8}, 0.0, 0.0);
        player.removeItem("rock");
        player.removeItem("stick");
    }
    else if (itemName == "armor" && player.hasItem("cloth") && player.hasItem("leather")) {
        crafted = player.craftItem("Armor",
            "a suit of armor made of cloth and leather scraps tied together with newts know-how",
            1.0, {5, 5, 5, 15}, 0.0, 0.0);
        player.removeItem("cloth");
        player.removeItem("leather");
    }

    if (crafted) {
        player.PickUpItem(crafted);
        std::cout << "\nYou watch as Newt crafts a " << crafted->getName() << " for you.\n";
        return true;
    }

    std::cout << "\nNewt: 'You don't have the materials for that.'\n";
    return false;
}
bool GameLoop::cmdSave(const std::string& filename) {
    return SaveManager::saveGame(filename, player, world);
}
bool GameLoop::cmdLoad(const std::string& filename) {
    return SaveManager::loadGame(filename, player, world);
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
bool GameLoop::checkWorldProgression() {
    WorldStage wl = world.getWorldLevel();
    int pl = player.getLevel();
    Room* loc = player.getLocation();
    if (!loc) return false;

    bool changed = false;

    // Stage 1 -> 2: talked Newt through to level 1 -> camp/shed/infirmary expand,
    // Minho + Pete show up.
    if (pl >= 1 && wl == WorldStage::Glade) {
        world.setWorldLevel(WorldStage::CampExpanded);
        world.createWorldLvlTwo();
        std::cout << "\n[The world shifts around you — things are changing...]\n";
        changed = true;
    }

    // Stage 2 -> 3: first trip into the maze with Alby + Minho -> First Griever
    // encounter (branch point: kill it or flee/betray).
    if (loc->getName() == "The Maze" && wl == WorldStage::CampExpanded) {
        world.setWorldLevel(WorldStage::MazeOpen);
        world.createMaze();
        world.createFirstGrieverEncounter();
        std::cout << "\n[The maze opens before you. Alby and Minho fall in step beside you...]\n";
        changed = true;
    }

    // Stage 3 -> 4: back at the Glade after the first encounter (killed or fled)
    // -> discover the safe zone is broken, the maze doesn't seal at night anymore.
    if (wl == WorldStage::MazeOpen && loc->getName() == "The Glade" &&
        (player.getFirstGrieverDefeated() || player.getBetrayedFriends())) {
        world.setWorldLevel(WorldStage::SafeZoneBroken);
        world.createSafeZoneBreach();
        std::cout << "\n[Something's wrong. The walls haven't moved. The Glade isn't safe anymore.]\n";
        changed = true;
    }

    // Stage 4 -> 5: Terrisa arrives, now through the Cage.
    if (wl == WorldStage::SafeZoneBroken && loc->getName() == "The Cage") {
        world.setWorldLevel(WorldStage::TerrisaArrives);
        world.createTerrisaArrival();
        std::cout << "\n[A girl stumbles out of the hatch, unconscious...]\n";
        changed = true;
    }

    // Stage 5 -> 6: Minho + Newt walk the player through the maze-phase map.
    if (wl == WorldStage::TerrisaArrives && pl >= 2) {
        world.setWorldLevel(WorldStage::MapRevealed);
        world.createMapReveal();
        std::cout << "\n[Minho and Newt lay out the map of the maze's phases for you.]\n";
        changed = true;
    }

    // Stage 6 -> 7: after more crafting/training, heading back into the maze
    // triggers the phase-two reconfiguration (new rooms + extra grievers).
    if (wl == WorldStage::MapRevealed && loc->getName() == "The Maze") {
        world.setWorldLevel(WorldStage::MazePhaseTwo);
        world.createMazePhaseTwo();
        std::cout << "\n[The maze reconfigures itself around you...]\n";
        changed = true;
    }

    // Stage 7 -> 8: player + Minho reach the sealed exit door.
    if (wl == WorldStage::MazePhaseTwo && loc->getName() == "Maze7") {
        world.setWorldLevel(WorldStage::MazeExitFound);
        std::cout << "\n[You and Minho find a massive sealed door. It won't budge — not yet.]\n";
        changed = true;
    }

    // Stage 8 -> 9: on the way back, the First Griever (or its corpse) again.
    if (wl == WorldStage::MazeExitFound && loc->getName() == "Maze5") {
        world.setWorldLevel(WorldStage::GrieverReturnEncounter);
        world.createGrieverReturnEncounter();
        if (player.getFirstGrieverDefeated())
            std::cout << "\n[You find the First Griever's corpse still slumped where you left it.]\n";
        else
            std::cout << "\n[The First Griever is still out there — and it's found you again.]\n";
        changed = true;
    }

    // Stage 9 -> 10: remains/venom brought back to camp, Terrisa pushes the choice.
    if (wl == WorldStage::GrieverReturnEncounter &&
        (loc->getName() == "The Camp Ground" || loc->getName() == "The Infirmary") &&
        (player.hasItem("Griever Remains") || player.getHarvestedVenom() || player.getFirstGrieverDefeated())) {
        world.setWorldLevel(WorldStage::FinalPreparation);
        world.createFinalPreparation();
        std::cout << "\n[Terrisa meets you at camp, eyeing what you brought back.]\n";
        changed = true;
    }

    // Stage 10 -> 11: player took the venom (EMP power) -> rally the helpers for
    // the final push into the maze.
    if (wl == WorldStage::FinalPreparation && player.hasMagic()) {
        world.setWorldLevel(WorldStage::FinalAssault);
        world.createFinalAssault();
        std::cout << "\n[The helpers gather. It's time to finish what the maze started.]\n";
        changed = true;
    }

    return changed;
}