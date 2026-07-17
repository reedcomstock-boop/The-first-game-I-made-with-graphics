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
    tilesets.emplace_back();
    if (!tilesets.back().load(path, 16)) {
        tilesets.pop_back();
        return -1;
    }
    int idx = (int)tilesets.size() - 1;
    tilesetIndex[name] = idx;
    return idx;
}

// A hand-authored room layout: each string is one row, each character maps to
// a tile via the legend below. Rows must all be the same length. Missing
// rooms fall back to the auto-generated border-box layout in buildLayouts().
struct ManualLayout {
    std::vector<std::string> rows;
    std::vector<std::string> decorRows;
    std::vector<DecorFeature> decorFeatures; // multi-cell decor objects (doors, banners, etc.)
    std::unordered_map<char, TileRef> legend;
    std::unordered_map<char, TileRef> decorLegend;
};

static std::unordered_map<std::string, ManualLayout> g_manualLayouts;

void RoomSceneManager::defineManualLayouts() {
    int floorsIdx  = tilesetIndex.count("floors")  ? tilesetIndex["floors"]  : -1;
    int wallsIdx   = tilesetIndex.count("walls")   ? tilesetIndex["walls"]   : -1;
    int waterIdx   = tilesetIndex.count("water")   ? tilesetIndex["water"]   : -1;
    int dungeonIdx = tilesetIndex.count("dungeon") ? tilesetIndex["dungeon"] : -1;

    // --- The Camp Ground (from earlier) ---
    ManualLayout campGround;
    campGround.legend = {
        { '#', { wallsIdx,  27 } },
        { '.', { floorsIdx, 16 } },
        { ',', { floorsIdx, 17 } },
        { '~', { waterIdx,   0 } },
    };
    campGround.rows = {
        "#####################",
        "#...................#",
        "#..,............~~..#",
        "#...................#",
        "#..,.......,....~~..#",
        "#...................#",
        "#####################",
    };
    g_manualLayouts["The Camp Ground"] = campGround;

   
    ManualLayout theCage;
    theCage.legend = {
        { '#', { dungeonIdx,2 /* solid wall col,row -> row*25+col */ } },
        { '&', { dungeonIdx,3 /* solid wall col,row -> row*25+col */ } },

        { '.', { dungeonIdx, 55/* solid floor col,row */ } },
    };
    theCage.rows = {
        "##########",
        "#........#",
        "#........#",
        "#........#",
        "##########",
    };
    theCage.decorLegend = {
        //feature{ 'D', { dungeonIdx, /* door tile col,row */ } },
        //{ 'S', { dungeonIdx, /* spike-bar tile col,row */ } },
        //feature{ 'B', { dungeonIdx, /* banner tile col,row */ } },
        { 'X', { dungeonIdx, 359/* blood stain tile col,row */ } },
        //feature{ 'T', { dungeonIdx, /* torch-topped wall col,row */ } },
        { ' ', { -1, -1 } }, // nothing — floor/wall shows through untouched
    };
    theCage.decorRows = {
        "TT######TT",
        "D........#",
        "#...B.....",
        "#....S...X",
        "##########",
    };
    theCage.decorFeatures = { { dungeonIdx, 0, 7, 2, 3, 3, 1 } }; // centered-ish, fits in 4 rows
    g_manualLayouts["The Cage"] = theCage;


}
void RoomSceneManager::loadTilesets(const std::string& assetDir) {
    registerTileset("floors",  assetDir + "/tiles/Floors_Tiles.png");
    registerTileset("walls",   assetDir + "/tiles/Wall_Tiles.png");
    registerTileset("water",   assetDir + "/tiles/Water_tiles.png");
    registerTileset("dungeon", assetDir + "/tiles/Dungeon_Tiles.png");
    defineManualLayouts(); // must come after tilesets are registered — needs their indices
}

void RoomSceneManager::loadNpcSprites(const std::string& assetDir) {
    // Newt — Knight skin (idle 4 frames @32x32, run 6 frames @64x64)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Knight/Idle-Sheet.png", 4, 0.15f, 32, 32);
        a.addClip("run",  assetDir + "/npc/Knight/Run-Sheet.png",  6, 0.10f, 64, 64);
        npcAnimators["Newt"] = std::move(a);
    }
    // Gally and Minho — Rogue skin, placeholder assignment for both
    for (const std::string name : {"Gally", "Minho"}) {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Rogue/Idle-Sheet.png", 4, 0.15f, 32, 32);
        a.addClip("run",  assetDir + "/npc/Rogue/Run-Sheet.png",  6, 0.10f, 64, 64);
        npcAnimators[name] = std::move(a);
    }
    // Pete — green-vest, side-facing only (idle 4 frames, walk 6 frames @64x64)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/pete/Idle_Side-Sheet.png", 4, 0.15f, 64, 64);
        a.addClip("walk", assetDir + "/npc/pete/Walk_Side-Sheet.png", 6, 0.10f, 64, 64);
        npcAnimators["Pete"] = std::move(a);
    }
}

void RoomSceneManager::loadProps(const std::string& assetDir) {
    {
        StripAnimator a;
        a.addClip("burn", assetDir + "/Props/Bonfire_01-Sheet.png", 4, 0.15f, 32, 32);
        propAnimators["bonfire"] = std::move(a);
    }
    {
        StripAnimator a;
        a.addClip("burn", assetDir + "/Props/Iron_01-Sheet.png", 2, 0.30f, 32, 96);
        propAnimators["forge_iron"] = std::move(a);
    }
}

void RoomSceneManager::loadMonsterSprites(const std::string& assetDir) {
    // Build a fresh animator per Griever name — can't share one StripAnimator
    // across map entries since it's move-only (each move empties the source).
    for (const std::string& n : {"The First Griever", "Griever 1", "Griever 2",
                                  "Griever 3", "Griever 5", "Griever 7", "Griever 8"}) {
        StripAnimator griever;
        griever.addClip("idle", assetDir + "/Mobs/Orc Crew/Orc - Warrior/Idle/Idle-Sheet.png", 4, 0.15f, 32, 32);
        griever.addClip("run",  assetDir + "/Mobs/Orc Crew/Orc - Warrior/Run/Run-Sheet.png",  6, 0.10f, 64, 64);
        npcAnimators[n] = std::move(griever);
    }
}


void RoomSceneManager::buildLayouts(const World& world, int cols, int rows) {
    int floorsIdx  = tilesetIndex.count("floors")  ? tilesetIndex["floors"]  : -1;
    int wallsIdx   = tilesetIndex.count("walls")   ? tilesetIndex["walls"]   : -1;
    int waterIdx   = tilesetIndex.count("water")   ? tilesetIndex["water"]   : -1;
    int dungeonIdx = tilesetIndex.count("dungeon") ? tilesetIndex["dungeon"] : -1;

    for (Room* r : world.getRooms()) {
        std::string name = r->getName();

        auto manualIt = g_manualLayouts.find(name);
        if (manualIt != g_manualLayouts.end()) {
            const ManualLayout& layout = manualIt->second;
            RoomScene scene;
            int layoutRows = (int)layout.rows.size();
            int layoutCols = layoutRows > 0 ? (int)layout.rows[0].size() : 0;
            scene.floor.assign(layoutRows, std::vector<TileRef>(layoutCols, {-1, -1}));

            for (int rr = 0; rr < layoutRows; rr++) {
                for (int cc = 0; cc < layoutCols && cc < (int)layout.rows[rr].size(); cc++) {
                    char ch = layout.rows[rr][cc];
                    auto legendIt = layout.legend.find(ch);
                    scene.floor[rr][cc] = (legendIt != layout.legend.end())
                        ? legendIt->second
                        : TileRef{ -1, -1 }; // unknown char — draws nothing, easy to spot while authoring
                }
            }
            scene.decor.assign(layoutRows, std::vector<TileRef>(layoutCols, {-1, -1}));
            for (int rr = 0; rr < layoutRows && rr < (int)layout.decorRows.size(); rr++) {
                for (int cc = 0; cc < layoutCols && cc < (int)layout.decorRows[rr].size(); cc++) {
                    char ch = layout.decorRows[rr][cc];
                    auto it = layout.decorLegend.find(ch);
                    scene.decor[rr][cc] = (it != layout.decorLegend.end()) ? it->second : TileRef{-1,-1};
                }
            }
            scene.decorFeatures = layout.decorFeatures;   // <-- add this line
            rooms[name] = scene;
            continue; // skip the auto-generated version entirely for this room
        }

        // --- existing auto-generated fallback for every other room ---
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
            if (t.tilesetId < 0 || t.tilesetId >= (int)tilesets.size()) continue;

            if (tilesets[t.tilesetId].isLoaded()) {
                tilesets[t.tilesetId].drawTile(t.tileIndex,
                    originX + c * tileDraw, originY + r * tileDraw, scale);
            } else {
                DrawRectangle((int)(originX + c * tileDraw), (int)(originY + r * tileDraw),
                              tileDraw, tileDraw, TILESET_COLORS[t.tilesetId]);
            }
        }
    }
}
void RoomSceneManager::drawDecor(const std::string& roomName, int originX, int originY, float scale) const {
    auto it = rooms.find(roomName);
    if (it == rooms.end()) return;
    const RoomScene& scene = it->second;

    int tileDraw = (int)(16 * scale);
    for (size_t r = 0; r < scene.decor.size(); r++) {
        for (size_t c = 0; c < scene.decor[r].size(); c++) {
            const TileRef& t = scene.decor[r][c];
            if (t.tilesetId < 0) continue; // no decor here — see the floor tile beneath
            tilesets[t.tilesetId].drawTile(t.tileIndex,
                originX + c * tileDraw, originY + r * tileDraw, scale);
        }
    }
}
void RoomSceneManager::drawDecorFeatures(const std::string& roomName, int originX, int originY, float scale) const {
    auto it = rooms.find(roomName);
    if (it == rooms.end()) return;

    int tileDraw = (int)(16 * scale);
    for (const DecorFeature& f : it->second.decorFeatures) {
        if (f.tilesetId < 0 || f.tilesetId >= (int)tilesets.size()) continue;
        tilesets[f.tilesetId].drawRegion(f.sheetCol, f.sheetRow, f.cellsWide, f.cellsHigh,
            originX + f.gridCol * tileDraw, originY + f.gridRow * tileDraw, scale);
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