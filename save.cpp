#include "save.h"
#include "player.h"
#include "world.h"
#include "rooms.h"
#include "items.h"
#include "stats.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

// Advances a freshly-constructed World (only createWorld() called so far, as
// main.cpp always does) forward through every stage up to and including
// targetStage, replaying the same create*() calls checkWorldProgression()
// would have triggered along the way. Needed because a save file can drop
// the player straight into a late-game room/stage whose rooms/NPCs wouldn't
// exist yet otherwise.
static void advanceWorldToStage(World& world, int targetStage) {
    int current = static_cast<int>(world.getWorldLevel());

    if (targetStage >= 2 && current < 2) {
        world.createWorldLvlTwo();
        world.setWorldLevel(WorldStage::CampExpanded);
        current = 2;
    }
    if (targetStage >= 3 && current < 3) {
        world.createMaze();
        world.createFirstGrieverEncounter();
        world.setWorldLevel(WorldStage::MazeOpen);
        current = 3;
    }
    if (targetStage >= 4 && current < 4) {
        world.createSafeZoneBreach();
        world.setWorldLevel(WorldStage::SafeZoneBroken);
        current = 4;
    }
    if (targetStage >= 5 && current < 5) {
        world.createTerrisaArrival();
        world.setWorldLevel(WorldStage::TerrisaArrives);
        current = 5;
    }
    if (targetStage >= 6 && current < 6) {
        world.createMapReveal();
        world.setWorldLevel(WorldStage::MapRevealed);
        current = 6;
    }
    if (targetStage >= 7 && current < 7) {
        world.createMazePhaseTwo();
        world.setWorldLevel(WorldStage::MazePhaseTwo);
        current = 7;
    }
    if (targetStage >= 8 && current < 8) {
        world.setWorldLevel(WorldStage::MazeExitFound);
        current = 8;
    }
    if (targetStage >= 9 && current < 9) {
        world.createGrieverReturnEncounter();
        world.setWorldLevel(WorldStage::GrieverReturnEncounter);
        current = 9;
    }
    if (targetStage >= 10 && current < 10) {
        world.createFinalPreparation();
        world.setWorldLevel(WorldStage::FinalPreparation);
        current = 10;
    }
    if (targetStage >= 11 && current < 11) {
        world.createFinalAssault();
        world.setWorldLevel(WorldStage::FinalAssault);
        current = 11;
    }
}

static std::vector<std::string> splitPipe(const std::string& line) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : line) {
        if (c == '|') { parts.push_back(cur); cur.clear(); }
        else cur += c;
    }
    parts.push_back(cur);
    return parts;
}

bool SaveManager::saveGame(const std::string& filename, Player& player, const World& world) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cout << "Could not open '" << filename << "' for saving.\n";
        return false;
    }

    out << "STAGE " << static_cast<int>(world.getWorldLevel()) << "\n";
    out << "NIGHTUNSAFE " << (world.isNightUnsafe() ? 1 : 0) << "\n";
    out << "NAME " << player.getName() << "\n";
    out << "DESC " << player.getDescription() << "\n";
    out << "HEALTH " << player.getHealth() << "\n";
    out << "ENERGY " << player.getEnergy() << "\n";
    out << "LEVEL " << player.getLevel() << "\n";
    out << "EXP " << player.getExp() << "\n";
    out << "EXPNEXT " << player.getExpToNextLevel() << "\n";
    out << "MAGIC " << (player.hasMagic() ? 1 : 0) << "\n";
    out << "DIALOGUE " << player.getDiologueProgress() << "\n";
    out << "GRIEVERDEFEATED " << (player.getFirstGrieverDefeated() ? 1 : 0) << "\n";
    out << "BETRAYED " << (player.getBetrayedFriends() ? 1 : 0) << "\n";
    out << "HARVESTEDVENOM " << (player.getHarvestedVenom() ? 1 : 0) << "\n";

    const Stats& s = player.getStats();
    out << "STATS " << s.strength << " " << s.dexterity << " " << s.intelligence << " " << s.defence << "\n";
    out << "LOCATION " << (player.getLocation() ? player.getLocation()->getName() : "") << "\n";

    const auto& items = player.getItems();
    out << "ITEMCOUNT " << items.size() << "\n";
    for (Item* item : items) {
        const Tool* tool = dynamic_cast<const Tool*>(item);
        if (tool) {
            const Stats& ts = tool->getStats();
            out << "TOOL|" << tool->getName() << "|" << tool->getDescription() << "|"
                << tool->getLevel() << "|" << ts.strength << "|" << ts.dexterity << "|"
                << ts.intelligence << "|" << ts.defence << "|"
                << tool->getHealth() << "|" << tool->getEnergy() << "\n";
        } else {
            out << "ITEM|" << item->getName() << "|" << item->getDescription() << "\n";
        }
    }

    const auto& equipped = player.getInventory();
    out << "EQUIPCOUNT " << equipped.size() << "\n";
    for (Item* item : equipped) {
        out << "EQUIP|" << item->getName() << "\n";
    }

    std::cout << "Game saved to '" << filename << "'.\n";
    return true;
}

bool SaveManager::loadGame(const std::string& filename, Player& player, World& world) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cout << "Could not open '" << filename << "' to load.\n";
        return false;
    }

    int stage = 1;
    bool nightUnsafe = false;
    std::string name, desc, locationName;
    double health = player.getHealth(), energy = player.getEnergy();
    double exp = 0.0, expNext = 100.0;
    int32_t level = 0, dialogue = 0;
    bool magic = false, grieverDefeated = false, betrayed = false, harvestedVenom = false;
    Stats stats = player.getStats();

    std::vector<std::string> itemLines;
    std::vector<std::string> equipNames;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        if (line.rfind("TOOL|", 0) == 0 || line.rfind("ITEM|", 0) == 0) {
            itemLines.push_back(line);
            continue;
        }
        if (line.rfind("EQUIP|", 0) == 0) {
            equipNames.push_back(line.substr(6));
            continue;
        }

        std::istringstream ls(line);
        std::string key;
        ls >> key;
        std::string rest;
        std::getline(ls, rest);
        if (!rest.empty() && rest[0] == ' ') rest = rest.substr(1);

        if      (key == "STAGE")           stage = std::stoi(rest);
        else if (key == "NIGHTUNSAFE")     nightUnsafe = (rest == "1");
        else if (key == "NAME")            name = rest;
        else if (key == "DESC")            desc = rest;
        else if (key == "HEALTH")          health = std::stod(rest);
        else if (key == "ENERGY")          energy = std::stod(rest);
        else if (key == "LEVEL")           level = std::stoi(rest);
        else if (key == "EXP")             exp = std::stod(rest);
        else if (key == "EXPNEXT")         expNext = std::stod(rest);
        else if (key == "MAGIC")           magic = (rest == "1");
        else if (key == "DIALOGUE")        dialogue = std::stoi(rest);
        else if (key == "GRIEVERDEFEATED") grieverDefeated = (rest == "1");
        else if (key == "BETRAYED")        betrayed = (rest == "1");
        else if (key == "HARVESTEDVENOM")  harvestedVenom = (rest == "1");
        else if (key == "STATS") {
            std::istringstream ss2(rest);
            ss2 >> stats.strength >> stats.dexterity >> stats.intelligence >> stats.defence;
        }
        else if (key == "LOCATION")   locationName = rest;
        // ITEMCOUNT / EQUIPCOUNT are informational only — the vectors above
        // are already sized by how many TOOL|/ITEM|/EQUIP| lines were read.
    }

    // --- Apply loaded state ---
    advanceWorldToStage(world, stage);
    world.setNightUnsafe(nightUnsafe);

    if (!name.empty()) player.setName(name);
    if (!desc.empty()) player.setDescription(desc);
    player.setHealth(health);
    player.setEnergy(energy);
    player.setStats(stats);
    player.setMagic(magic);
    player.setDiologueProgress(dialogue);
    player.setFirstGrieverDefeated(grieverDefeated);
    player.setBetrayedFriends(betrayed);
    player.setHarvestedVenom(harvestedVenom);
    player.restoreProgress(level, exp, expNext);

    if (!locationName.empty()) {
        Room* room = world.getRoomByName(locationName);
        if (room) player.setLocation(room);
    }

    // Rebuild inventory
    std::vector<std::pair<std::string, Item*>> byName;
    for (const std::string& itemLine : itemLines) {
        std::vector<std::string> f = splitPipe(itemLine);
        if (f.empty()) continue;

        if (f[0] == "ITEM" && f.size() >= 3) {
            Item* item = new Item(f[1], f[2]);
            player.PickUpItem(item);
            byName.push_back({f[1], item});
        } else if (f[0] == "TOOL" && f.size() >= 10) {
            Stats ts;
            double toolLevel = std::stod(f[3]);
            ts.strength     = std::stod(f[4]);
            ts.dexterity    = std::stod(f[5]);
            ts.intelligence = std::stod(f[6]);
            ts.defence      = std::stod(f[7]);
            double toolHealth = std::stod(f[8]);
            double toolEnergy = std::stod(f[9]);
            Tool* tool = new Tool(f[1], f[2], toolLevel, ts, toolHealth, toolEnergy);
            player.PickUpItem(tool);
            byName.push_back({f[1], tool});
        }
    }

    for (const std::string& eqName : equipNames) {
        for (auto& kv : byName) {
            if (kv.first == eqName) {
                Tool* tool = dynamic_cast<Tool*>(kv.second);
                if (tool) player.equipItem(tool);
                break;
            }
        }
    }

    std::cout << "Game loaded from '" << filename << "'.\n";
    return true;
}