#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <string>
#include "rooms.h"
#include "Entity.h"

enum class WorldStage : int32_t {
    Glade                  = 1,  // Wake in the Cage -> Glade opened up (tour w/ Alby, run around, explore)
    CampExpanded           = 2,  // Talked Newt through his full dialogue tree -> camp/shed/infirmary expand, Minho+Pete appear
    MazeOpen               = 3,  // Entered the maze w/ Alby+Minho -> first Griever encounter (branch point)
    SafeZoneBroken         = 4,  // Returned to Glade -> maze no longer closes to monsters at night, safe zone is gone
    TerrisaArrives         = 5,  // Terrisa stumbles out of the Cage (moved from Walls per new story order)
    MapRevealed            = 6,  // Minho + Newt show the player the map of the maze's phases
    MazePhaseTwo           = 7,  // More crafting/training done -> maze reconfigures, phase-two rooms + grievers open
    MazeExitFound           = 8,  // Player + Minho find the sealed exit door they can't open
    GrieverReturnEncounter = 9,  // On the way back: fight the First Griever again, OR find/harvest its corpse (branch)
    FinalPreparation       = 10, // Remains brought to camp; Terrisa pushes the venom/EMP choice
    FinalAssault           = 11  // Player leads the helpers into the maze to the exit -> ending
};

class World {
public:
    World();
    ~World();

    void createWorld();
    void createWorldLvlTwo();
    void createMaze();
    void createFirstGrieverEncounter();   // stage 3: places the First Griever for the initial maze run
    void createSafeZoneBreach();          // stage 4: glade/camp no longer safe at night
    void createTerrisaArrival();          // stage 5: Terrisa now arrives in The Cage
    void createMapReveal();               // stage 6: Minho/Newt show the maze-phase map (mostly dialogue-gated)
    void createMazePhaseTwo();            // stage 7: existing maze expansion, now gated behind MapRevealed + prep
    void createGrieverReturnEncounter();  // stage 9: branches on whether the First Griever was killed earlier
    void createFinalPreparation();        // stage 10: remains brought back, venom/EMP choice staged
    void createFinalAssault();            // stage 11: helpers join the player for the final push

    NPC* getNpcByName(const std::string& name) const;
    Room* getRoomByName(const std::string& name) const;

    WorldStage getWorldLevel() const;
    void setWorldLevel(WorldStage stage);
    const std::vector<Room*>& getRooms() const;

    // Branch-relevant world state, checked by GameLoop/NPC dialogue stubs
    bool isNightUnsafe() const { return nightUnsafe; }
    void setNightUnsafe(bool v) { nightUnsafe = v; }
    Monster* getFirstGriever() const { return firstGriever; }

private:
    std::vector<Room*> rooms;
    WorldStage worldLevel;
    Helper* newt = nullptr;
    Helper* gally = nullptr;
    Helper* minho = nullptr;
    Helper* terrisa = nullptr;
    Helper* alby = nullptr;
    Medic* pete = nullptr;

    Monster* firstGriever = nullptr; // tracked so the return encounter knows if it's alive or a corpse
    bool nightUnsafe = false;        // true once SafeZoneBroken hits
};

#endif // WORLD_H