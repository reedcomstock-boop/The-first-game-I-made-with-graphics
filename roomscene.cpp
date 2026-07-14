#include "roomscene.h"

// Pixel Crawler's tile sheets are built entirely from autotile connector
// pieces (crosses, hooks, corners meant to combine into a matched edge set) —
// there is no single index in any of these sheets that renders as a flat
// fill on its own. Rather than fight that, draw a solid color per tileset
// type; registration order in loadTilesets() is floors=0, walls=1, water=2,
// dungeon=3.
static const Color TILESET_COLORS[4] = {
    { 46, 92, 40, 255 },    // floors — grass green
    { 90, 78, 64, 255 },    // walls — stone/brown
    { 40, 90, 140, 255 },   // water — blue
    { 35, 35, 42, 255 },    // dungeon — dark stone
};

// Tile picks — index into each 16x16 tileset grid (row*columns+col).
// Pixel Crawler's tilesets are built for autotiling: most tiles are "island"
// or "connector" pieces that are only solid in part of the 16x16 cell (a
// blob, a cross/plus shape, etc.) and are meant to combine with matching
// edge pieces. Repeating any single one of those alone produces dots or
// crosses. These four were verified by checking that all four edges of the
// tile are opaque (not just the tile overall) and by rendering each one
// tiled 3x3 to confirm no seams/gaps before picking it.
static const int FLOOR_TILE_INDEX   = 16;  // Floors_Tiles.png  row 0, col 16 — solid, tiles clean
static const int WALL_TILE_INDEX    = 27;  // Wall_Tiles.png    row 1, col 2  — solid, tiles clean
static const int WATER_TILE_INDEX   = 0;   // Water_tiles.png   row 0, col 0  — solid, tiles clean
static const int DUNGEON_TILE_INDEX = 0;   // Dungeon_Tiles.png row 0, col 0  — solid, tiles clean
static const int DUNGEON_WALL_INDEX = 0;

RoomSceneManager::RoomSceneManager() {}
RoomSceneManager::~RoomSceneManager() {}

int RoomSceneManager::registerTileset(const std::string& name, const std::string& path) {
    TileSet ts;
    ts.load(path, 16);
    tilesets.push_back(ts);
    int idx = (int)tilesets.size() - 1;
    tilesetIndex[name] = idx;
    return idx;
}

void RoomSceneManager::loadTilesets(const std::string& assetDir) {
    registerTileset("floors",  assetDir + "/tiles/Floors_Tiles.png");
    registerTileset("walls",   assetDir + "/tiles/Wall_Tiles.png");
    registerTileset("water",   assetDir + "/tiles/Water_tiles.png");
    registerTileset("dungeon", assetDir + "/tiles/Dungeon_Tiles.png");
}

void RoomSceneManager::loadNpcSprites(const std::string& assetDir) {
    // Newt — Knight skin (idle 4 frames @32x32, run 6 frames @64x64)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Knight/Idle-Sheet.png", 4, 0.15f, 32, 32);
        a.addClip("run",  assetDir + "/npc/Knight/Run-Sheet.png",  6, 0.10f, 64, 64);
        npcAnimators["Newt"] = a;
    }
    // Gally and Minho — Rogue skin, placeholder assignment for both
    for (const std::string name : {"Gally", "Minho"}) {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Rogue/Idle-Sheet.png", 4, 0.15f, 32, 32);
        a.addClip("run",  assetDir + "/npc/Rogue/Run-Sheet.png",  6, 0.10f, 64, 64);
        npcAnimators[name] = a;
    }
    // Pete — green-vest, side-facing only (idle 4 frames, walk 6 frames @64x64)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/pete/Idle_Side-Sheet.png", 4, 0.15f, 64, 64);
        a.addClip("walk", assetDir + "/npc/pete/Walk_Side-Sheet.png", 6, 0.10f, 64, 64);
        npcAnimators["Pete"] = a;
    }
}

void RoomSceneManager::loadProps(const std::string& assetDir) {
    {
        StripAnimator a;
        a.addClip("burn", assetDir + "/Props/Bonfire_01-Sheet.png", 4, 0.15f, 32, 32);
        propAnimators["bonfire"] = a;
    }
    {
        StripAnimator a;
        a.addClip("burn", assetDir + "/Props/Iron_01-Sheet.png", 2, 0.30f, 32, 96);
        propAnimators["forge_iron"] = a;
    }
}

void RoomSceneManager::buildLayouts(const World& world, int cols, int rows) {
    int floorsIdx  = tilesetIndex.count("floors")  ? tilesetIndex["floors"]  : -1;
    int wallsIdx   = tilesetIndex.count("walls")   ? tilesetIndex["walls"]   : -1;
    int waterIdx   = tilesetIndex.count("water")   ? tilesetIndex["water"]   : -1;
    int dungeonIdx = tilesetIndex.count("dungeon") ? tilesetIndex["dungeon"] : -1;

    for (Room* r : world.getRooms()) {
        std::string name = r->getName();

        int useFloorSet  = floorsIdx;
        int useFloorTile = FLOOR_TILE_INDEX;
        int useWallSet   = wallsIdx;
        int useWallTile  = WALL_TILE_INDEX;

        if (name == "The Lake") {
            useFloorSet  = waterIdx;
            useFloorTile = WATER_TILE_INDEX;
        } else if (name == "The Maze" || name.rfind("Maze", 0) == 0) {
            useFloorSet  = dungeonIdx;
            useFloorTile = DUNGEON_TILE_INDEX;
            useWallSet   = dungeonIdx;
            useWallTile  = DUNGEON_WALL_INDEX;
        }

        RoomScene scene;
        scene.floor.assign(rows, std::vector<TileRef>(cols, { useFloorSet, useFloorTile }));
        for (int c = 0; c < cols; c++) {
            scene.floor[0][c]        = { useWallSet, useWallTile };
            scene.floor[rows - 1][c] = { useWallSet, useWallTile };
        }
        for (int rr = 0; rr < rows; rr++) {
            scene.floor[rr][0]        = { useWallSet, useWallTile };
            scene.floor[rr][cols - 1] = { useWallSet, useWallTile };
        }

        rooms[name] = scene;
    }

    // Themed prop placement — the only two rooms called out in the asset list.
    if (rooms.count("The Camp Ground"))
        rooms["The Camp Ground"].props.push_back({ "bonfire", 0.5f, 0.6f });
    if (rooms.count("The Shed"))
        rooms["The Shed"].props.push_back({ "forge_iron", 0.5f, 0.55f });
}

void RoomSceneManager::update(float dt) {
    for (auto& kv : npcAnimators)  kv.second.update(dt);
    for (auto& kv : propAnimators) kv.second.update(dt);
}

void RoomSceneManager::unloadAll() {
    for (auto& ts : tilesets) ts.unload();
    for (auto& kv : npcAnimators)  kv.second.unload();
    for (auto& kv : propAnimators) kv.second.unload();
}

void RoomSceneManager::drawFloor(const std::string& roomName, int originX, int originY, float scale) const {
    auto it = rooms.find(roomName);
    if (it == rooms.end()) return;
    const RoomScene& scene = it->second;

    int tileDraw = (int)(16 * scale);
    for (size_t r = 0; r < scene.floor.size(); r++) {
        for (size_t c = 0; c < scene.floor[r].size(); c++) {
            const TileRef& t = scene.floor[r][c];
            if (t.tilesetId < 0 || t.tilesetId >= 4) continue;
            DrawRectangle((int)(originX + c * tileDraw), (int)(originY + r * tileDraw),
                          tileDraw, tileDraw, TILESET_COLORS[t.tilesetId]);
        }
    }
}

void RoomSceneManager::drawProps(const std::string& roomName, int originX, int originY,
                                  int viewportW, int viewportH, float scale) const {
    auto it = rooms.find(roomName);
    if (it == rooms.end()) return;

    for (const PropInstance& p : it->second.props) {
        auto animIt = propAnimators.find(p.clipName);
        if (animIt == propAnimators.end()) continue;
        int x = originX + (int)(p.relX * viewportW);
        int y = originY + (int)(p.relY * viewportH);
        animIt->second.draw(x, y, scale);
    }
}
void RoomSceneManager::loadMonsterSprites(const std::string& assetDir) {
    StripAnimator griever;
    griever.addClip("idle", assetDir + "/Mobs/Orc Crew/Orc - Warrior/Idle/Idle-Sheet.png", 4, 0.15f, 32, 32);
    griever.addClip("run",  assetDir + "/Mobs/Orc Crew/Orc - Warrior/Run/Run-Sheet.png",  6, 0.10f, 64, 64);

    // Register under every Griever name used in world.cpp
    for (const std::string n : {"The First Griever", "Griever 1", "Griever 2",
                                  "Griever 3", "Griever 5", "Griever 7", "Griever 8"}) {
        npcAnimators[n] = griever;
    }
}

void RoomSceneManager::drawNpcs(const std::vector<NPC*>& npcsInRoom, int originX, int originY,
                                 int viewportW, int viewportH, float scale) const {
    if (npcsInRoom.empty()) return;

    // Evenly spread whoever is actually in the room along a fixed line in the
    // upper part of the scene — kept separate from Thomas's own draw line
    // (0.65 of viewport height, set in graphics.cpp) so sprites don't overlap.
    int count = (int)npcsInRoom.size();
    int y = originY + (int)(viewportH * 0.30f);
    for (int i = 0; i < count; i++) {
        auto it = npcAnimators.find(npcsInRoom[i]->getName());
        if (it == npcAnimators.end()) continue; // no sprite mapped for this NPC yet
        int x = originX + (int)(viewportW * (float)(i + 1) / (float)(count + 1));
        it->second.draw(x, y, scale);
    }
}