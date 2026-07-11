#include "world.h"
#include "items.h"

World::World() : worldLevel(1) {}

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
int World::getWorldLevel() const { return worldLevel; }
void World::setWorldLevel(int level) { worldLevel = level; }
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
    if (TrainingGrounds) TrainingGrounds->addNpcEntity(minho);

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
        if (shed) shed->addNpcEntity(newt);
    }


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

    if (glade)      rock->putInRoom(glade);
    if (campGround) stick->putInRoom(campGround);
    if (campGround) leather->putInRoom(campGround);
    if (glade)      cloth->putInRoom(glade);
    if (infirmary)  metal->putInRoom(infirmary);
    if (infirmary)  medpack->putInRoom(infirmary);
    if (shed)       rations->putInRoom(shed);
    if (TrainingGrounds) TrainingSword->putInRoom(TrainingGrounds);
    if (Lake)       rock->putInRoom(Lake);
    if (TheWoods)   stick->putInRoom(TheWoods);
    if (TreeHouse)  Map->putInRoom(TreeHouse);
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

    Stats grieverstats = {8, 3, 2, 6};
    Monster* griever1 = new Monster("The First Griever", "A hulking mechanical beast.", 100.0, grieverstats, 50.0);
    Maze5->addNpcEntity(griever1);
    
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
