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

// ---------------------------------------------------------------------
// Dungeon_Tiles.png catalog — indices are col/row within the 25x25 grid.
// TileRef fills use the flat index (row*25+col); DecorFeature objects use
// a {col,row,w,h} rect that gets combined with dungeonIdx + a grid
// placement at the point of use (see theCage for the pattern).
// ---------------------------------------------------------------------

// --- Repeatable TileRef fills (row*25+col) ---
static const int DUNGEON_WALL_INDEX        = 2;   // row0,col2  — solid brick wall
static const int DUNGEON_WALL_CORNER_INDEX = 3;   // row0,col3  — stepped wall-top/corner variant
static const int DUNGEON_FLOOR_INDEX       = 55;  // row2,col5  — solid floor

static const int DUNGEON_RAIL_CAP_INDEX    = 14 * 25 + 0; // row14,col0 — top rail, any zone
static const int DUNGEON_RAIL_ORANGE_INDEX = 16 * 25 + 1; // row16,col1 — railing body, orange zone
static const int DUNGEON_RAIL_BLUE_INDEX   = 16 * 25 + 4; // row16,col4 — railing body, blue zone
static const int DUNGEON_RAIL_GREEN_INDEX  = 16 * 25 + 7; // row16,col7 — railing body, green zone

// --- Single-cell decor (for decorLegend, drawn via drawDecor) ---
static const int DUNGEON_RIVET_INDEX       = 2 * 25 + 8;   // row2,col8  — 4-dot rivet cluster
static const int DUNGEON_STAIN_A_INDEX     = 13 * 25 + 6;  // row13,col6 — blood stain variant A
static const int DUNGEON_STAIN_B_INDEX     = 13 * 25 + 7;  // row13,col7 — blood stain variant B

// --- Multi-cell decor (for decorFeatures, drawn via drawDecorFeatures) ---
struct FeatureRect { int col, row, w, h; };

static const FeatureRect DUNGEON_DOOR_ARCH        = { 0,  7, 2, 3 }; // existing — matched double archway
static const FeatureRect DUNGEON_PRISON_BARS      = { 9,  3, 1, 8 }; // tall cell bars
static const FeatureRect DUNGEON_CELL_DOOR        = { 8,  3, 1, 7 }; // paneled door w/ hinges, pairs with bars
static const FeatureRect DUNGEON_VENT_CONSOLE     = { 0,  2, 3, 2 };
static const FeatureRect DUNGEON_BULLETIN_BOARD   = { 17, 0, 3, 3 };
static const FeatureRect DUNGEON_HATCH_FRAME      = { 21, 1, 2, 2 };
static const FeatureRect DUNGEON_CRATE_RACK_LG    = { 17, 3, 3, 3 };
static const FeatureRect DUNGEON_CRATE_RACK_SM    = { 20, 4, 3, 2 };
static const FeatureRect DUNGEON_BENCH            = { 0,  13, 4, 1 };
static const FeatureRect DUNGEON_CRACKED_WALL     = { 8,  13, 1, 4 };
static const FeatureRect DUNGEON_ORB_ORANGE       = { 0,  19, 3, 3 };
static const FeatureRect DUNGEON_ORB_ORANGE_CAP   = { 1,  18, 1, 1 };
static const FeatureRect DUNGEON_ORB_BLUE         = { 4,  19, 3, 3 };
static const FeatureRect DUNGEON_ORB_BLUE_CAP     = { 5,  18, 1, 1 };
static const FeatureRect DUNGEON_ORB_GREEN        = { 8,  19, 3, 3 };
static const FeatureRect DUNGEON_ORB_GREEN_CAP    = { 9,  18, 1, 1 };

// Flagged as approximate — verify visually before relying on these:
static const FeatureRect DUNGEON_BANNER_APPROX    = { 4,  10, 1, 4 };
static const FeatureRect DUNGEON_FOLIAGE_APPROX   = { 12, 0,  2, 5 };
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
static DecorFeature DF(int tilesetId, const FeatureRect& r, int gridCol, int gridRow) {
    return { tilesetId, r.col, r.row, r.w, r.h, gridCol, gridRow };
}

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
        { '#', { dungeonIdx, DUNGEON_WALL_INDEX } },
        { '&', { dungeonIdx, DUNGEON_WALL_CORNER_INDEX } },
        { '.', { dungeonIdx, DUNGEON_FLOOR_INDEX } },
        { ' ', { -1, -1 } },
    };
    theCage.decorLegend = {
        { 'X', { dungeonIdx, DUNGEON_STAIN_A_INDEX } },
        { 'r', { dungeonIdx, DUNGEON_RIVET_INDEX } },
        { ' ', { -1, -1 } },
    };
    theCage.decorFeatures = {
        DF(dungeonIdx, DUNGEON_DOOR_ARCH,   6, 1),  // the door, same placement as before
        DF(dungeonIdx, DUNGEON_PRISON_BARS, 1, 0),  // bars along the west wall
        DF(dungeonIdx, DUNGEON_CELL_DOOR,   6, 1),  // paired cell door next to the bars
    };
    theCage.rows = {
        "#####################",
        "#..............##",
        "#..............##",
        "#..............##",
        "#..............##",
        "#..............##",
        "#####################",
    };
    
    /*theCage.decorRows = {
        " TT######TT",
        " D........#",
        " #...B.....",
        " #....S...X",
        " ##########",
    };*/
    g_manualLayouts["The Cage"] = theCage;


}

void RoomSceneManager::loadTilesets(const std::string& assetDir) {
    registerTileset("floors",  assetDir + "/tiles/Floors_Tiles.png");
    registerTileset("walls",   assetDir + "/tiles/Wall_Tiles.png");
    registerTileset("water",   assetDir + "/tiles/Water_tiles.png");
    registerTileset("dungeon", assetDir + "/tiles/Dungeon_Tiles.png");
    defineManualLayouts(); // must come after tilesets are registered — needs their indices
}

void RoomSceneManager::loadNpcPortraits(const std::string& assetDir) {
    npcPortraits["Alby"] = LoadTexture((assetDir + "/npc/Alby_new/alby_face.png").c_str());
}

Texture2D RoomSceneManager::getPortrait(const std::string& name) const {
    auto it = npcPortraits.find(name);
    if (it != npcPortraits.end()) return it->second;
    return Texture2D{};   // id=0 signals "no portrait" — caller checks before drawing
}

void RoomSceneManager::loadNpcSprites(const std::string& assetDir) {
    // Newt — Knight skin (idle 4 frames @32x32, run 6 frames @64x64)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Newt/newt_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Newt/newt_walk.png", 3, 0.10f, 48, 48, 3);

        npcAnimators["Newt"] = std::move(a);
    }
    // Gally — Rogue skin
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Gally/Gally_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Gally/Gally_walk.png", 3, 0.10f, 48, 48, 3);
        npcAnimators["Gally"] = std::move(a);
    }
    // Minho — Male Adventurer pack (48x64 frames, 8 per animation)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Minho/Minho_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Minho/Minho_walk.png", 3, 0.10f, 48, 48, 3);
        npcAnimators["Minho"] = std::move(a);
    }
    // Terrisa — Female Adventurer pack (48x64 frames, 8 per animation)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Terrisa/Alby_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Terrisa/Alby_walking.png", 3, 0.10f, 48, 48, 3);
        npcAnimators["Terrisa"] = std::move(a);
    }
    // Alby — using the new Alby_walking.png / Alby_hurt.png sheets (RPG Maker MZ
    // layout, same format as Thomas's tommy_* sheets). Only the Down-facing row
    // is used since NPCs here don't turn to face movement direction — same
    // convention as Newt/Minho/etc.'s existing single-direction sheets.
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Alby/Alby_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Alby/Alby_walking.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Alby/Alby_hurt.png",    3, 0.15f, 48, 48, 3);
        npcAnimators["Alby"] = std::move(a);
    }
    // Pete — Archaeologist sheet (idle row, 64x32 frames, 8 per row)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Pete/Pete_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Pete/Pete_walking.png", 3, 0.10f, 48, 48, 3);
        a.addClip("attack",  assetDir + "/npc/Pete/Attack.png", 4, 0.15f, 64, 32);
        
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
    for (const std::string n : {"The First Griever", "Griever 1", "Griever 2",
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
    for (auto& kv : npcPortraits)  if (kv.second.id != 0) UnloadTexture(kv.second);

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
    int count = (int)npcsInRoom.size();
    int y = originY + (int)(viewportH * 0.30f);

    for (int i = 0; i < count; i++) {
        // 1. Guard against null pointers in the room vector
        if (npcsInRoom[i] == nullptr) continue; 

        // 2. Fetch the name safely 
        std::string name = npcsInRoom[i]->getName();                            

        // 3. Look up the animator
        auto it = npcAnimators.find(name);
        if (it == npcAnimators.end()) continue; 

        // 4. Calculate coordinates and render cleanly
        int x = originX + (int)(viewportW * (float)(i + 1) / (float)(count + 1));
        it->second.draw(x, y, scale/2.0f);
    }
}
