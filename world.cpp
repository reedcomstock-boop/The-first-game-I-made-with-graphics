#include "world.h"
#include "items.h"

World::World() : worldLevel(WorldStage::Glade) {}

World::~World() {
    for (Room* r : rooms) delete r;
}
NPC* World::getNpcByName(const std::string& name) const {
    for (Room* r : rooms) {
        for (NPC* npc : r->getNpcEntities()) {
            if (npc->getName() == name) return npc;
        }
    }
    return nullptr;
}
WorldStage World::getWorldLevel() const { return worldLevel; }

void World::setWorldLevel(WorldStage stage) { worldLevel = stage; }

const std::vector<Room*>& World::getRooms() const { return rooms; }

Room* World::getRoomByName(const std::string& name) const {
    for (Room* r : rooms) {
        if (r->getName() == name) return r;
    }
    return nullptr;
}

void World::createWorld() {
    // --- Build rooms ---

    Room* startRoom  = new Room("The Cage",
        "You wake up in a metal box filled with haphazardly packaged supplies for a campground.",
        {}, {});
    alby = new Helper("Alby", "A wise old runner who has seen many things. He's been stuck in the maze for longer than anyone else.", 100.0, {8,8,8,8});
    Room* glade      = new Room("The Glade",
        "You look around and see that you are in an open grassy field surrounded by tall stone walls.",
        {}, {});
    newt = new Helper("Newt", "A helpful runner who can give you advice and help you craft items.", 100.0, {9,9,9,9});
    Room* campGround = new Room("The Camp Ground",
        "You walk toward the camp grounds east of the field and see many tents to sleep in. "
        "To the south, you see an old looking shed, and to the east there is what looks to be an infirmary. "
        "Newt has a lot to talk to you about, make sure to see what all he has to say.",
        {}, {});
    gally = new Helper("Gally", "A suspicious runner who is always on the lookout to teach new runners some sense.", 100.0,{7,7,7,7});
    Room* walls      = new Room("The Walls",
        "You walk towards the walls and hear a subtle shifting of gears within them.",
        {}, {});
    Room* Maze       = new Room("The Maze",
        "You enter The Maze.",
        {}, {});
    Room* shed       = new Room("The Shed",
        "You walk over to see a run down old shed as you head south from the camp ground.",
        {}, {});
    Room* infirmary  = new Room("The Infirmary",
        "You walk towards the shack that looks to have been expanded by the people in the glade. "
        "There is a giant cross outside, this could be the glade's medical center.",
        {}, {});
    Room* Lake = new Room("The Lake",
        "You walk towards the Lake and see a large body of water. The water is clear and you can see fish swimming around.",
        {}, {});
    Room* TrainingGrounds = new Room("The Training Grounds",
        "You walk towards the training grounds and see a large open area with various obstacles and targets. "
        "This is where the runners train to be the best.",
        {}, {});
    Room* TheWoods= new Room("The Woods",
        "You walk towards the woods and see a dense forest. The trees are tall and the canopy is thick, making it hard to see the sky.",
        {}, {});
    Room* TreeHouse= new Room("The Tree House",
        "You look up in the trees from the woods and see what looks to be a ladder leading up to a tree house. You climb up and see a small room with a bed and a desk.",
        {}, {});

    // --- Connect rooms ---
    Room::connectRooms(startRoom,"up",glade,"down");
    Room::connectRooms(glade,"east",campGround,"west");
    Room::connectRooms(glade,"north", walls,"south");
    Room::connectRooms(walls,"north", Maze,"south");
    Room::connectRooms(campGround,"north", shed,"south");
    Room::connectRooms(campGround,"east",infirmary,"west");
    Room::connectRooms(glade,"south",Lake,"north");
    Room::connectRooms(glade,"west",TrainingGrounds,"east");
    Room::connectRooms(TheWoods,"up",TreeHouse,"down");

    // --- Place starting items ---
    Item* rock    = new Item("Rock",      "This is just a rock. Can be used in crafting.");
    Item* stick   = new Item("Stick",     "This is just a sturdy stick. Can be used in crafting.");
    Item* metal   = new Item("Metal",     "This is just a piece of metal. Can be used in crafting.");
    Tool* rations = new Tool("Rations",   "Rations to keep you and the rest of the runners from starving.",0, {0, 0, 5, 0}, 5.0, 15.0);
    Tool* medpack = new Tool("Med packs", "Medical supplies to keep you from getting too close to death.",0, {0, 0, 0, 5}, 20.0, 5.0);
    
    // --- Place npcs in rooms ---
  
    campGround->addNpcEntity(newt);
    walls->addNpcEntity(gally);
    glade->addNpcEntity(alby);


    // --- Place items in rooms ---
    rations->putInRoom(startRoom);
    medpack->putInRoom(startRoom);
    rock->putInRoom(shed);
    stick->putInRoom(infirmary);
    metal->putInRoom(campGround);

    // --- Register all rooms ---
    rooms = { startRoom, glade, campGround, walls, Maze, shed, infirmary, Lake, TrainingGrounds, TheWoods, TreeHouse };
}
void World::createWorldLvlTwo() {
    Room* shed      = getRoomByName("The Shed");
    Room* infirmary = getRoomByName("The Infirmary");
    Room* campGround= getRoomByName("The Camp Ground");
    Room* glade     = getRoomByName("The Glade");
    Room* TrainingGrounds = getRoomByName("The Training Grounds");
    Room* Lake      = getRoomByName("The Lake");
    Room* TheWoods  = getRoomByName("The Woods");
    Room* TreeHouse = getRoomByName("The Tree House");

    // Add Pete the medic to the infirmary
    pete = new Medic("Pete", "A friendly medic who can heal you and maybe help you regain your memories.", 100.0, {5, 5, 5, 5});

    minho = new Helper("Minho", "A skilled runner who can teach you advanced combat techniques.", 100.0, {8, 8, 8, 8});
    TrainingGrounds->addNpcEntity(minho);

    // Update room descriptions
    if (TreeHouse){
        TreeHouse->setDescription("You walk in to see a rundown-looking loft with a complex "
            "three dimensional map of The Maze made from sticks on top of a large round "
            "table completely covering it.");
        }
    if (infirmary){
        infirmary->setDescription("You walk in to see a rather homely building with a couple "
            "of comfortable sofas. As soon as you walk in you feel your health has been rejuvenated.");
         infirmary->addNpcEntity(pete);
        }
    if (shed){
        shed->setDescription("You walk in to see a small workshop with a couple of workbenches. "
            "There are a few tools and scrapped materials scattered around the room.");
        if (shed){ 
            shed->addNpcEntity(newt);
        }
    }
    campGround->removeNpcEntity(newt);


    // Spawn extra crafting materials around the world
    Item* leather = new Item("Leather", "A scrap of leather. Can be used in crafting.");
    Item* cloth   = new Item("Cloth",   "A scrap of cloth. Can be used in crafting.");
    Item* rock    = new Item("Rock",    "Just a rock. Can be used in crafting.");
    Item* stick   = new Item("Stick",   "A sturdy stick. Can be used in crafting.");
    Item* metal   = new Item("Metal",   "A piece of metal. Can be used in crafting.");
    Tool* rations = new Tool("Rations", "Rations to keep you going.", 0, {0, 0, 5, 0}, 5.0, 15.0);
    Tool* medpack = new Tool("Med packs", "Medical supplies.", 0, {0, 0, 0, 3}, 20.0, 5.0);
    Tool* TrainingSword = new Tool("Training Sword", "A sword to help you train and get better at fighting.", 0, {5, 0, 0, 0}, 0.0, 3.0);
    Item* Map = new Item("Map", "A map of the maze to help you navigate.");

    rock->putInRoom(glade);
    stick->putInRoom(campGround);
    leather->putInRoom(campGround);
    cloth->putInRoom(glade);
    metal->putInRoom(infirmary);
    medpack->putInRoom(infirmary);
    rations->putInRoom(shed);
    TrainingSword->putInRoom(TrainingGrounds);
    rock->putInRoom(Lake);
    stick->putInRoom(TheWoods);
    Map->putInRoom(TreeHouse);
}
void World::createMaze(){
    Room* Maze = getRoomByName("The Maze");
    if(!Maze) return;
    
    Room* Maze1 = new Room("Maze1","A towering stone corridor stretches endlessly ahead, walls slick with moss and cold moisture. Faint echoes suggest distant movement deep within the Maze.",{},{});
    Room* Maze2 = new Room("Maze2","A narrow passage twists sharply, dim light barely illuminating claw-like scratches along the walls. The air feels heavy, almost watchful and hostile.",{},{});
    Room* Maze3 = new Room("Maze3", "The corridor opens slightly, revealing tangled vines gripping the stone. A chilling draft sweeps through, carrying metallic scents from unseen machinery.",{},{});
    Room* Maze4 = new Room("Maze4","A wide chamber with jagged stones jutting from the floor. Strange mechanical hums vibrate through the walls, making your heartbeat feel painfully loud.",{},{});
    Room* Maze5 = new Room("Maze5","A long hallway stretches forward, lined with deep gouges from past battles. Shadows lengthen unnaturally, flickering as if something moves just out of sight.",{},{});
    
    Room* Maze6 = new Room("Maze6", "The path curves through a suffocatingly tight stone channel. Dust drifts in the air, disturbed by a slow, rhythmic thudding from somewhere ahead.",{},{});
    Room* Maze7 = new Room("Maze7","You reach a dead-end chamber with a massive sealed door. Cool air leaks from beneath it, hinting at the Maze’s true exit—yet unreachable.",{},{});
    Room* Maze8 = new Room("Maze8","A strangely calm pocket of the Maze. Soft moss carpets the floor, though the silence feels staged—too perfect, like a trap waiting patiently.",{},{});

    Room::connectRooms(Maze,  "north", Maze1, "south");
    Room::connectRooms(Maze1, "west",  Maze2, "east");
    Room::connectRooms(Maze1, "north", Maze3, "south");
    Room::connectRooms(Maze2, "west",  Maze8, "east");
    Room::connectRooms(Maze3, "north", Maze4, "south");
    Room::connectRooms(Maze3, "east",  Maze5, "west");
    Room::connectRooms(Maze5, "east",  Maze6, "west");
    Room::connectRooms(Maze6, "south", Maze7, "north");

    rooms.insert(rooms.end(), {Maze1, Maze2, Maze3, Maze4, Maze5, Maze6, Maze7, Maze8});
}

void World::createFirstGrieverEncounter() {
    // Stage 3: MazeOpen. Player goes in with Alby + Minho and meets the First Griever.
    // Outcome (kill vs. flee/betray) is recorded on Player and read back in
    // createGrieverReturnEncounter() later.
    Room* Maze5 = getRoomByName("Maze5");
    if (!Maze5) return;

    Stats grieverStats = {8, 3, 2, 6};
    firstGriever = new Monster("The First Griever", "A hulking mechanical beast.", 100.0, grieverStats, 50.0);
    Maze5->addNpcEntity(firstGriever);

    // TODO(dialogue): Alby/Minho escort commentary before/after this fight.
}

void World::createSafeZoneBreach() {
    // Stage 4: player + Minho + Alby return to the Glade to find the maze doesn't
    // seal itself off from monsters at night anymore -- the safe zone is gone.
    nightUnsafe = true;

    Room* glade = getRoomByName("The Glade");
    Room* campGround = getRoomByName("The Camp Ground");
    if (glade) {
        glade->setDescription(glade->getDescription() +
            " Something's changed -- the walls don't shift shut at night anymore. "
            "People are on edge, taking shifts on watch.");
    }
    if (campGround) {
        campGround->setDescription(campGround->getDescription() +
            " The camp feels tense now that the Glade isn't sealed off at night.");
    }

    // TODO: spawn a night-Griever encounter in the Glade/campground rooms once
    // a day/night mechanic exists; structurally just flagged via nightUnsafe for now.
}
void World::createTerrisaArrival() {
    // Stage 5: moved from The Walls to The Cage per the new story order -- she
    // comes up through the same hatch Thomas did.
    Room* cage = getRoomByName("The Cage");
    if (!cage) return;

    terrisa = new Helper("Terrisa",
        "A girl who stumbled out of the Cage, unconscious, and somehow knows your name.",
        100.0, {6, 8, 9, 5});

    cage->addNpcEntity(terrisa);
    cage->setDescription("The hatch groans open again, out of cycle. A girl tumbles out, "
        "unconscious -- the first girl anyone's ever seen come up. Terrisa stirs nearby.");
}
void World::createMapReveal() {
    // Stage 6: mostly a dialogue beat (Minho + Newt walk the player through the
    // tree-house map of the maze's phases). No room changes needed yet --
    // hook is here so GameLoop/NPC.cpp have a stage to gate that conversation on.
    // TODO(dialogue): Newt/Minho dialogue in TreeHouse referencing the Map item.
}

void World::createGrieverReturnEncounter() {
    // Stage 9: branches on whether the player killed the First Griever back in
    // MazeOpen (stage 3) or fled/betrayed their friends instead.
    Room* Maze5 = getRoomByName("Maze5");
    if (!Maze5) return;

    if (firstGriever && firstGriever->getHealth() > 0) {
        // Still alive (player fled earlier) -- second, forced confrontation.
        // TODO(dialogue): Alby/Minho remark on the betrayal before the fight.
    } else {
        // Player killed it already -- leave a harvestable corpse instead.
        Item* remains = new Item("Griever Remains",
            "The husk of the First Griever, venom sacs still intact. Could be harvested.");
        remains->putInRoom(Maze5);
        // TODO(dialogue): Alby/Minho remark on finding the corpse, offer to harvest.
    }
}

void World::createFinalPreparation() {
    // Stage 10: remains/venom brought back to camp; Terrisa pushes the player
    // to take it, regain memories, and gain EMP power.
    // TODO(dialogue): Terrisa/Pete conversation gating player.setMagic(true).
}

void World::createFinalAssault() {
    // Stage 11: all the helpers join the player for the push to the exit.
    Room* mazeExit = getRoomByName("Maze7"); // sealed door room, found back in stage 8
    if (!mazeExit) return;

    for (Helper* h : {newt, gally, minho, terrisa, alby}) {
        if (!h) continue;
        // Pull them out of wherever they were and stage them at the exit room
        // for the final push. Room removal isn't tracked per-NPC here, so this
        // just ensures they're present at the finale.
        mazeExit->addNpcEntity(h);
    }
    // TODO: ending trigger (e.g. GameLoop detects player + all helpers in Maze7
    // and prints the victory ending instead of the "you fall" default ending).
}
void World:: createMazePhaseTwo(){
    Room* Maze  = getRoomByName("The Maze");
    Room* Maze3 = getRoomByName("Maze3");
    Room* Maze4 = getRoomByName("Maze4");
    Room* Maze5 = getRoomByName("Maze5");
    Room* Maze6 = getRoomByName("Maze6");
    Room* Maze7 = getRoomByName("Maze7");
    Room* Maze8 = getRoomByName("Maze8");
    if (!Maze) return;
    // New rooms added in phase two
    Room* Maze9  = new Room("Maze9",  "Dark mechanical chamber with grinding gears.", {}, {});
    Room* Maze10  = new Room("Maze10", "Long slanted shaft covered in metal plates.", {}, {});
    Room* Maze11  = new Room("Maze11", "Circular node room with pulsing lights.", {}, {});
    Room* Maze12  = new Room("Maze12", "Dusty hall full of strange metallic debris.", {}, {});
    Room* Maze13  = new Room("Maze13", "Collapsed room with a narrow crawling path.", {}, {});
    Room* Maze14  = new Room("Maze14", "Large echoing tunnel with flickering lights.", {}, {});

    // Note: "The First Griever" is no longer spawned here -- it's placed earlier
    // by createFirstGrieverEncounter() (stage 3) so its fate (killed/fled) can be
    // tracked into the branch-aware return encounter later.

    // Add grievers to existing Maze rooms
    Stats grieverStats = {6, 3, 2, 4};
    if (Maze4) Maze4->addNpcEntity(new Monster("Griever 1", "A hulking mechanical beast.", 60.0, grieverStats, 30.0));
    if (Maze6) Maze6->addNpcEntity(new Monster("Griever 2", "A hulking mechanical beast.", 60.0, grieverStats, 30.0));
    if (Maze7) Maze7->addNpcEntity(new Monster("Griever 3", "A hulking mechanical beast.", 60.0, grieverStats, 30.0));
    if (Maze8) Maze8->addNpcEntity(new Monster("Griever 5", "A hulking mechanical beast.", 60.0, grieverStats, 30.0));
    if (Maze3) Maze3->addNpcEntity(new Monster("Griever 7", "A hulking mechanical beast.", 60.0, grieverStats, 30.0));
    if (Maze5) Maze5->addNpcEntity(new Monster("Griever 8", "A hulking mechanical beast.", 60.0, grieverStats, 30.0));

    // Connect new rooms
    Room::connectRooms(Maze12, "north", Maze14, "south");
    Room::connectRooms(Maze14, "west",  Maze10, "east");
    Room::connectRooms(Maze10, "west",  Maze9, "east");
    Room::connectRooms(Maze9, "south", Maze11, "north");
    Room::connectRooms(Maze11, "south", Maze13, "north");
    if (Maze4) Room::connectRooms(Maze4, "north", Maze12, "south");
    if (Maze7) Room::connectRooms(Maze13, "east",  Maze7,  "west");

    rooms.insert(rooms.end(), {Maze9, Maze10, Maze11, Maze12, Maze13, Maze14});

    // Suppress unused variable warning if Maze9 was retrieved but not used
    (void)Maze9;
}