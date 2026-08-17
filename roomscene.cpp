#include "roomscene.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
 
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


// Per-room Tiled map cache. Keyed by room name, so any number of rooms can
// each load their own .tmx independently — this used to be a single set of
// globals hardcoded to "The Camp Ground", meaning loading a second Tiled
// room would silently overwrite the first one's data (broken collision,
// broken doors, blank rendering for whichever room loaded first).
struct TiledRoomData {
    std::vector<int> ground, walls, water, collision;
    int width = 0, height = 0;
    std::vector<TiledDoor> doors;
    std::vector<PropInstance> props;   // NEW
    bool loaded = false;
    std::vector<std::pair<int,int>> gidRanges;
};
static std::unordered_map<std::string, TiledRoomData> g_tiledRooms;

// Turns a Room's display name into the .tmx filename this project expects,
// e.g. "The Camp Ground" -> "assets/maps/The_Camp_Ground.tmx". Keep new
// rooms' names/files consistent with this (spaces -> underscores, same
// capitalization) or buildLayouts() won't find the map and will silently
// fall back to the auto-generated box layout.
static std::string tiledMapPathForRoom(const std::string& roomName) {
    std::string fname = roomName;
    for (char& c : fname) if (c == ' ') c = '_';
    return "assets/maps/" + fname + ".tmx";
}
 
// ---------------------------------------------------------------------
// Dungeon_Tiles.png catalog — indices are col/row within the 25x25 grid.
// Every coordinate below was verified by cropping the actual PNG with PIL
// and viewing the result — do not trust a gridded screenshot over this.
// TileRef fills use the flat index (row*25+col); DecorFeature objects use
// a {col,row,w,h} rect that gets combined with dungeonIdx + a grid
// placement at the point of use (see theCage for the pattern).
// ---------------------------------------------------------------------
 
// --- Repeatable TileRef fills (row*25+col) ---
static const int DUNGEON_WALL_INDEX        = 2;
static const int DUNGEON_WALL_CORNER_INDEX = 3;
static const int DUNGEON_FLOOR_INDEX       = 55;
 
static const int DUNGEON_RAIL_CAP_INDEX    = 14 * 25 + 0; // row14,col0 — top rail, any zone
static const int DUNGEON_RAIL_ORANGE_INDEX = 16 * 25 + 1; // row16,col1 — railing body, orange zone
static const int DUNGEON_RAIL_BLUE_INDEX   = 16 * 25 + 4; // row16,col4 — railing body, blue zone
static const int DUNGEON_RAIL_GREEN_INDEX  = 16 * 25 + 7; // row16,col7 — railing body, green zone
 
// --- Single-cell decor ---
static const int DUNGEON_RIVET_INDEX       = 2 * 25 + 8;
static const int DUNGEON_STAIN_A_INDEX     = 13 * 25 + 6;
// static const int DUNGEON_STAIN_B_INDEX  = 13 * 25 + 7; // row13,col7 — blood stain variant B
 
// --- Multi-cell decor ---
struct FeatureRect { int col, row, w, h; };
 
// Confirmed by direct pixel crop: arched stone doorway w/ wood panel.
[[maybe_unused]] static const FeatureRect DUNGEON_DOOR_ARCH = { 0, 7, 2, 3 };
 
// Was {8,3,1,7} — row3 is still background, the door art doesn't start
// until row4 and is only 6 rows tall, not 7. Fixed via gridded crop.
static const FeatureRect DUNGEON_CELL_DOOR =  { 7, 6, 2, 4 };
 
// Was one rect {9,3,1,8} spanning TWO unrelated graphics with a blank/
// transparent gap in between (straight bars end at row7, gate lattice
// starts at row8) — that gap is what rendered as the broken/see-through
// bar artifact in-game. Split into the two real assets.
static const FeatureRect DUNGEON_PRISON_BARS_STRAIGHT = { 9, 4, 1, 4 }; // solid vertical bars
static const FeatureRect DUNGEON_PRISON_GATE_LATTICE  = { 9, 8, 1, 2 }; // crossed grate, separate asset
 
static const FeatureRect DUNGEON_VENT_CONSOLE     = { 0,  2, 3, 2 };
static const FeatureRect DUNGEON_BULLETIN_BOARD   = { 17, 0, 3, 3 };
static const FeatureRect DUNGEON_HATCH_FRAME      = { 21, 1, 2, 2 };
static const FeatureRect DUNGEON_CRATE_RACK_LG    = { 17, 3, 3, 3 };
static const FeatureRect DUNGEON_CRATE_RACK_SM    = { 20, 4, 3, 2 };
static const FeatureRect DUNGEON_BENCH            = { 0,  13, 4, 1 };
static const FeatureRect DUNGEON_CRACKED_WALL     = { 8,  13, 1, 4 };
 
// Orb clusters (bottom of sheet). Blue/green were off by one column vs the
// orange one — confirmed by cropping cols 0-12, rows 17-22 with a grid.
// Currently unused by any room; left here (commented) for when needed.
 static const FeatureRect DUNGEON_ORB_ORANGE     = { 0, 19, 3, 3 };
 static const FeatureRect DUNGEON_ORB_ORANGE_CAP = { 1, 18, 1, 1 };
// static const FeatureRect DUNGEON_ORB_BLUE       = { 3, 19, 3, 3 };  // was {4,19,3,3}
// static const FeatureRect DUNGEON_ORB_BLUE_CAP   = { 4, 18, 1, 1 };  // was {5,18,1,1}
// static const FeatureRect DUNGEON_ORB_GREEN      = { 6, 19, 3, 3 };  // was {8,19,3,3}
// static const FeatureRect DUNGEON_ORB_GREEN_CAP  = { 7, 18, 1, 1 };  // was {9,18,1,1}
 
// Approximate — verify visually before uncommenting/using:
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
// Overload: accept a flat tile index (row*25+col) for single-cell features.
static DecorFeature DF(int tilesetId, int flatTileIndex, int gridCol, int gridRow) {
    int col = flatTileIndex % 25;
    int row = flatTileIndex / 25;
    return { tilesetId, col, row, 1, 1, gridCol, gridRow };
}

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
    //DF(dungeonIdx, DUNGEON_DOOR_ARCH,            6, 1),
    //DF(dungeonIdx, DUNGEON_CELL_DOOR,            4, 1),  // barred gate, next to the door
    DF(dungeonIdx, DUNGEON_PRISON_BARS_STRAIGHT, 1, 0),
    DF(dungeonIdx, DUNGEON_PRISON_GATE_LATTICE,  9, 0),
    DF(dungeonIdx, DUNGEON_ORB_ORANGE,           1, 2),
    DF(dungeonIdx, DUNGEON_ORB_ORANGE_CAP,       2, 1),
    DF(dungeonIdx, DUNGEON_ORB_ORANGE,           6, 2),
    DF(dungeonIdx, DUNGEON_ORB_ORANGE_CAP,       7, 1),  
    DF(dungeonIdx, DUNGEON_ORB_ORANGE,           12, 2),
    DF(dungeonIdx, DUNGEON_ORB_ORANGE_CAP,       13, 1),
    };
    theCage.rows = {
        "&###################&",
        "#..............##",
        "#..............##",
        "#..............##",
        "#..............##",
        "#..............##",
        "&###################&",
    };
    
    /*theCage.decorRows = {
        " TT######TT",
        " D........#",
        " #...B.....",
        " #....S...X",
        " ##########",
    };*/
    g_manualLayouts["The Cage"] = theCage;
ManualLayout theShed;
    theShed.legend = {
        { '#', { dungeonIdx, DUNGEON_WALL_INDEX } },
        { '&', { dungeonIdx, DUNGEON_WALL_CORNER_INDEX } },
        { '.', { dungeonIdx, DUNGEON_FLOOR_INDEX } },
    
        { ' ', { -1, -1 } },
    };
    theShed.decorLegend = {
        { 'X', { dungeonIdx, DUNGEON_STAIN_A_INDEX } },
        { 'r', { dungeonIdx, DUNGEON_RIVET_INDEX } },
        { ' ', { -1, -1 } },
    };
    theShed.decorFeatures = {
    DF(dungeonIdx, DUNGEON_VENT_CONSOLE, 1, 0),
    DF(dungeonIdx, DUNGEON_BULLETIN_BOARD, 3, 0),
    DF(dungeonIdx, DUNGEON_HATCH_FRAME, 5, 0),
    DF(dungeonIdx, DUNGEON_CRATE_RACK_LG, 7, 0),    
    DF(dungeonIdx, DUNGEON_CRATE_RACK_SM, 10, 0),
    DF(dungeonIdx, DUNGEON_BENCH, 13, 0),
    DF(dungeonIdx, DUNGEON_CRACKED_WALL, 15, 0),    
    DF(dungeonIdx, DUNGEON_RAIL_BLUE_INDEX, 1, 2),
    DF(dungeonIdx, DUNGEON_RAIL_GREEN_INDEX, 9, 2),
    DF(dungeonIdx, DUNGEON_RAIL_ORANGE_INDEX, 17, 2),
    DF(dungeonIdx, DUNGEON_RAIL_CAP_INDEX, 25, 1),
    DF(dungeonIdx, DUNGEON_FOLIAGE_APPROX, 1, 2),
    DF(dungeonIdx, DUNGEON_BANNER_APPROX, 5, 2),
    DF(dungeonIdx, DUNGEON_CELL_DOOR, 9, 2),

    };
    theShed.rows = {
        "&###################&",
        "#..............##",
        "#..............##",
        "#..............##",
        "#..............##",
        "#..............##",
        "&###################&",
    };
    
    /*theShed.decorRows = {
        " TT######TT",
        " D........#",
        " #...B.....",
        " #....S...X",
        " ##########",
    };*/
    g_manualLayouts["The Shed"] = theShed;

}
bool RoomSceneManager::isTileBlocked(const std::string& roomName,
                                      float relX, float relY) const {
    // Tiled-driven rooms check the real collision layer.
    auto tiledIt = g_tiledRooms.find(roomName);
    if (tiledIt != g_tiledRooms.end() && tiledIt->second.loaded &&
        tiledIt->second.width > 0 && tiledIt->second.height > 0)
    {
        const TiledRoomData& d = tiledIt->second;
        int tileCol = (int)(relX * d.width);
        int tileRow = (int)(relY * d.height);

        if (tileCol < 0 || tileCol >= d.width)  return true;
        if (tileRow < 0 || tileRow >= d.height) return true;

        int index = tileRow * d.width + tileCol;
        if (index < 0 || index >= (int)d.collision.size()) return false;

        return d.collision[index] != 0;   // any non-zero GID = blocked
    }

    // Everything else keeps using the existing ASCII manual layouts.
    auto it = g_manualLayouts.find(roomName);
    if (it == g_manualLayouts.end()) return false;

    const std::vector<std::string>& rows = it->second.rows;
    if (rows.empty()) return false;

    int numRows = (int)rows.size();
    int numCols = (int)rows[0].size();
    int tileCol = (int)(relX * numCols);
    int tileRow = (int)(relY * numRows);

    if (tileCol < 0 || tileCol >= numCols) return true;
    if (tileRow < 0 || tileRow >= numRows) return true;

    char cell = rows[tileRow][tileCol];
    return cell == '#' || cell == '&';
}
void RoomSceneManager::loadTilesets(const std::string& assetDir) {
    registerTileset("floors",  assetDir + "/tiles/Floors_Tiles.png");
    registerTileset("walls",   assetDir + "/tiles/Wall_Tiles.png");
    registerTileset("water",   assetDir + "/tiles/Water_tiles.png");
    registerTileset("dungeon", assetDir + "/tiles/Dungeon_Tiles.png");
    defineManualLayouts(); // must come after tilesets are registered — needs their indices
}

void RoomSceneManager::loadNpcPortraits(const std::string& assetDir) {
    npcPortraits["Alby"] = LoadTexture((assetDir + "/npc/Alby/Alby_face.png").c_str());
    npcPortraits["Gally"] = LoadTexture((assetDir + "/npc/Gally/Gally_face.png").c_str());
    npcPortraits["Minho"] = LoadTexture((assetDir + "/npc/Minho/Minho_face.png").c_str());
    npcPortraits["Terrisa"] = LoadTexture((assetDir + "/npc/Terrisa/Terrisa_face.png").c_str());
    npcPortraits["Newt"] = LoadTexture((assetDir + "/npc/Newt/Newt_face.png").c_str());
    npcPortraits["Pete"] = LoadTexture((assetDir + "/npc/Pete/Pete_face.png").c_str());

}

Texture2D RoomSceneManager::getPortrait(const std::string& name) const {
    auto it = npcPortraits.find(name);
    if (it != npcPortraits.end()) return it->second;
    return Texture2D{};   // id=0 signals "no portrait" — caller checks before drawing
}

void RoomSceneManager::loadNpcSprites(const std::string& assetDir) {
    // Newt
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Newt/newt_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Newt/newt_walk.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Newt/newt_hurt.png", 3, 0.15f, 48, 48, 3);
        npcAnimators["Newt"] = std::move(a);
    }
    // Gally
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Gally/Gally_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Gally/Gally_walking.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Gally/Gally_Hurt.png",    3, 0.15f, 48, 48, 3);
        npcAnimators["Gally"] = std::move(a);
    }
    // Minho
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Minho/Minho_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Minho/Minho_walk.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Minho/Minho_hurt.png", 3, 0.15f, 48, 48, 3);
        npcAnimators["Minho"] = std::move(a);
    }
    // Terrisa
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Terrisa/terissa_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Terrisa/terissa_walk.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Terrisa/terissa_hurt.png", 3, 0.15f, 48, 48, 3);
        npcAnimators["Terrisa"] = std::move(a);
    }
    // Alby
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Alby/Alby_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Alby/Alby_walking.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Alby/Alby_hurt.png",    3, 0.15f, 48, 48, 3);
        npcAnimators["Alby"] = std::move(a);
    }
    // Pete
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Pete/Pete_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Pete/Pete_walking.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Pete/Pete_hurt.png",    3, 0.15f, 48, 48, 3);
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



// ---------------------------------------------------------------------
// Tiled TMX loader
//
// The Tiled loader reads orthogonal CSV tile layers from a TMX map.
// The current map uses 16x16 tiles.
//
// Tiled uses a "global tile ID" (GID).  The GID includes the tileset's
// firstgid, so we convert it back into the local tile index used by the
// existing TileSet class.
// ---------------------------------------------------------------------

static bool loadTmxLayerCSV(const std::string& tmx,const std::string& layerName,int expectedWidth,int expectedHeight,std::vector<int>& out){
    std::regex layerRegex("<layer[^>]*name=\"" + layerName +"\"[^>]*>[\\s\\S]*?<data[^>]*encoding=\"csv\"[^>]*>([\\s\\S]*?)</data>",std::regex_constants::icase);

    std::smatch match;

    if (!std::regex_search(tmx, match, layerRegex)) {
        return false;
    }

    std::string csv = match[1].str();
    std::stringstream ss(csv);
    std::string value;

    out.clear();

    while (std::getline(ss, value, ',')) {
        value.erase(std::remove_if(value.begin(),value.end(),[](unsigned char c) {return std::isspace(c);}),value.end());
        if (!value.empty()) {
            out.push_back(std::stoi(value));
        }
    }

    return (int)out.size() == expectedWidth * expectedHeight;
}


// Reads every <tileset firstgid="N" source="....tsx"/> declaration out of
// a TMX file and maps each one, by filename, to a runtime tileset index.
// This is what lets each room's map declare its own tilesets in whatever
// order/combination it wants (e.g. Hut1.tmx has only Dungeon_Tiles at
// firstgid=1) instead of assuming every map shares The_Camp_Ground.tmx's
// exact firstgid layout (Floors=1, Dungeon=651, Wall=1276).
//
// We match on filename only, ignoring the directory portion of `source` —
// some exported maps have a broken/overlong relative path there, but that's
// a separate bug from figuring out which tileset a GID belongs to.
//
// Returned as {firstgid, runtimeIdx} pairs sorted ascending by firstgid.
static std::vector<std::pair<int,int>> parseTmxTilesetRanges(
        const std::string& tmx, int floorsIdx, int dungeonIdx,
        int wallsIdx, int waterIdx) {
    std::vector<std::pair<int,int>> ranges;

    std::regex tsRegex(R"RX(<tileset\s+firstgid="(\d+)"\s+source="([^"]*)")RX",
                        std::regex_constants::icase);

    for (std::sregex_iterator it(tmx.begin(), tmx.end(), tsRegex), end; it != end; ++it) {
        int firstgid = std::stoi((*it)[1].str());
        std::string source = (*it)[2].str();

        std::string filename = source;
        size_t slash = filename.find_last_of("/\\");
        if (slash != std::string::npos) filename = filename.substr(slash + 1);

        int idx = -1;
        if (filename.find("Floors") != std::string::npos)       idx = floorsIdx;
        else if (filename.find("Dungeon") != std::string::npos) idx = dungeonIdx;
        else if (filename.find("Wall") != std::string::npos)    idx = wallsIdx;
        else if (filename.find("Water") != std::string::npos)   idx = waterIdx;

        if (idx != -1) {
            ranges.push_back({firstgid, idx});
        } else {
            std::cerr << "[Tiled] Unrecognized tileset source '" << source
                       << "' — tiles from this tileset will not render.\n";
        }
    }

    std::sort(ranges.begin(), ranges.end());
    return ranges;
}

// Convert a Tiled global tile ID into the local tile index used by TileSet,
// using this map's own tileset ranges (see parseTmxTilesetRanges) rather
// than a hardcoded set of GID boundaries.
//
// Tiled's high bits are reserved for horizontal/vertical/diagonal flips.
// We do not currently use flipped tiles, but mask those bits so a flipped
// tile will not turn into an invalid tile index.
static TileRef tiledGidToTileRef(int gid, const std::vector<std::pair<int,int>>& ranges){
    // Empty cell.
    if (gid <= 0 || ranges.empty()) {
        return {-1, -1};
    }

    // Remove Tiled flip flags.
    const int TILED_GID_MASK = 0x1FFFFFFF;
    gid &= TILED_GID_MASK;

    // ranges is sorted ascending by firstgid — find the last range whose
    // firstgid is <= gid, i.e. the tileset this GID actually belongs to.
    int chosenIdx = -1;
    int chosenFirstgid = 1;
    for (const auto& range : ranges) {
        if (range.first <= gid) {
            chosenIdx = range.second;
            chosenFirstgid = range.first;
        } else {
            break;
        }
    }

    if (chosenIdx == -1) {
        return {-1, -1};
    }
    return {chosenIdx, gid - chosenFirstgid};
}

// Parses <objectgroup name="doors"> ... <object .../> ... </objectgroup>.
// Assumes Tiled's standard attribute order (id, name, x, y, width, height) —
// this is not a general TMX parser, just enough for this project's export.
static void loadTmxDoors(const std::string& tmx, int mapWidthPx, int mapHeightPx,std::vector<TiledDoor>& out)
{
    out.clear();

    std::regex groupRegex(
        "<objectgroup[^>]*name=\"doors\"[^>]*>([\\s\\S]*?)</objectgroup>",
        std::regex_constants::icase);
    std::smatch groupMatch;
    if (!std::regex_search(tmx, groupMatch, groupRegex)) return;
    std::string block = groupMatch[1].str();

    // Custom delimiter "RX" is required here — the regex itself contains a
    // literal `)"` sequence (in `([^"]*)"`), which is identical to a plain
    // R"( ... )" delimiter's closing tag. With no custom tag, the raw string
    // terminates right there instead of at the real end, and everything
    // after gets parsed as broken C++ ("stray '\' in program" errors).
    std::regex objRegex(
        R"RX(<object\s+id="\d+"\s+name="([^"]*)"\s+x="([-\d.]+)"\s+y="([-\d.]+)"\s+width="([-\d.]+)"\s+height="([-\d.]+)"\s*>([\s\S]*?)</object>)RX");

    for (std::sregex_iterator it(block.begin(), block.end(), objRegex), end; it != end; ++it) {
        std::smatch m = *it;
        TiledDoor d;
        d.name = m[1].str();
        float x = std::stof(m[2].str());
        float y = std::stof(m[3].str());
        float w = std::stof(m[4].str());
        float h = std::stof(m[5].str());
        std::string props = m[6].str();

        d.x0 = x / (float)mapWidthPx;
        d.y0 = y / (float)mapHeightPx;
        d.x1 = (x + w) / (float)mapWidthPx;
        d.y1 = (y + h) / (float)mapHeightPx;

        std::smatch pm;
        // Same raw-string delimiter collision as objRegex above — these all
        // contain a `)"` inside the pattern itself (end of a capture group
        // right before a literal quote), so a plain R"( ... )" truncates
        // early and even swallows the group's closing paren. Custom "RX"
        // delimiter avoids the collision entirely.
        std::regex targetRegex(R"RX(name="targetName"\s+value="([^"]*)")RX");
        if (std::regex_search(props, pm, targetRegex)) d.targetRoom = pm[1].str();

        std::regex spawnXRegex(R"RX(name="spawnx"[^>]*value="([-\d.]+)")RX");
        if (std::regex_search(props, pm, spawnXRegex)) d.spawnX = std::stof(pm[1].str());
        else d.spawnX = 0.5f;

        std::regex spawnYRegex(R"RX(name="spawny"[^>]*value="([-\d.]+)")RX");
        if (std::regex_search(props, pm, spawnYRegex)) d.spawnY = std::stof(pm[1].str());
        else d.spawnY = 0.9f;

        out.push_back(d);
        std::cout << "[DEBUG DOOR] name='" << d.name << "' target='" << d.targetRoom << "' rect=(" << d.x0 << "," << d.y0 << ")-(" << d.x1 << "," << d.y1 << ")\n";
    }
}
// Parses <objectgroup name="props"> ... <object name="clipName" x=".." y=".."/> ...
// Point/rect objects placed in Tiled under a "props" layer. The object's
// `name` must match a clip registered in RoomSceneManager::loadProps()
// (e.g. "bonfire", "forge_iron"). Position is stored as a 0..1 fraction of
// the map so any room size works the same way doors already do.
static void loadTmxProps(const std::string& tmx, int mapWidthPx, int mapHeightPx,
                          std::vector<PropInstance>& out)
{
    out.clear();

    std::regex groupRegex(
        "<objectgroup[^>]*name=\"props\"[^>]*>([\\s\\S]*?)</objectgroup>",
        std::regex_constants::icase);
    std::smatch groupMatch;
    if (!std::regex_search(tmx, groupMatch, groupRegex)) return;
    std::string block = groupMatch[1].str();

    std::regex objRegex(
        R"RX(<object\s+id="\d+"\s+name="([^"]*)"\s+x="([-\d.]+)"\s+y="([-\d.]+)")RX");

    for (std::sregex_iterator it(block.begin(), block.end(), objRegex), end; it != end; ++it) {
        std::smatch m = *it;
        std::string clipName = m[1].str();
        if (clipName.empty()) continue;

        float x = std::stof(m[2].str());
        float y = std::stof(m[3].str());

        PropInstance p;
        p.clipName = clipName;
        p.relX = x / (float)mapWidthPx;
        p.relY = y / (float)mapHeightPx;
        out.push_back(p);
    }
}
static bool loadTiledRoom(const std::string& path, const std::string& roomName,
                           RoomScene& scene, int floorsIdx, int dungeonIdx, int wallsIdx,
                           int waterIdx){
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr<< "[Tiled] Could not open map: "<< path << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string tmx = buffer.str();

    // Read the map dimensions directly from the TMX file.  This means you
    // can resize the map in Tiled without changing C++ code.
    std::regex mapRegex(R"TMX(<map\b[^>]*\bwidth="([0-9]+)"[^>]*\bheight="([0-9]+)")TMX",std::regex_constants::icase);

    std::smatch mapMatch;
    if (!std::regex_search(tmx, mapMatch, mapRegex)) {
        std::cerr << "[Tiled] Could not read map dimensions.\n";
        return false;
    }

    const int width = std::stoi(mapMatch[1].str());
    const int height = std::stoi(mapMatch[2].str());

    if (width <= 0 || height <= 0) {
        std::cerr << "[Tiled] Invalid map dimensions: " << width << "x" << height << "\n";
        return false;
    }
    std::vector<int> ground;
    std::vector<int> walls;
    std::vector<int> water;
    std::vector<int> decor;
    std::vector<int> collision;
    if (!loadTmxLayerCSV(tmx, "ground", width, height, ground)){
        std::cerr<< "[Tiled] Could not read ground layer.\n";
        return false;
    }
    if (!loadTmxLayerCSV(tmx, "walls", width, height, walls)){
    std::cerr << "[Tiled] Could not read walls layer in " << path << "\n";
    std::cerr << "[Tiled] Map size: " << width << "x" << height << "\n";
    return false;
    }   
    // These layers are optional for this first test.
    loadTmxLayerCSV(tmx, "water", width, height, water);
    loadTmxLayerCSV(tmx, "decor", width, height, decor);
    loadTmxLayerCSV(tmx, "collision", width, height, collision); 
    // RoomScene's existing renderer expects the floor array to contain the
    // visible base tile for every cell.  For this first integration pass,
    // we flatten the visible Tiled ground/walls/water layers into that array.
    scene.floor.assign(height,std::vector<TileRef>(width,{-1, -1}));
    scene.decor.assign(height,std::vector<TileRef>(width,{-1, -1}));
    // -------------------------------------------------------------
    // Preserve Tiled's layers separately.
    //
    // The old test flattened walls/water into scene.floor. That caused a
    // transparent wall tile to replace the ground underneath it, so the
    // black viewport showed through wherever the overlay was transparent.
    //
    // We now retain the layers and draw them in order:
    // ground -> water -> walls -> decor.
    // -------------------------------------------------------------
    TiledRoomData& data = g_tiledRooms[roomName];
    data.ground     = ground;
    data.walls      = walls;
    data.water      = water;
    data.width      = width;
    data.height     = height;
    data.collision  = collision;
    data.loaded     = true;
    data.gidRanges  = parseTmxTilesetRanges(tmx, floorsIdx, dungeonIdx, wallsIdx, waterIdx);
    // Keep these arrays populated for compatibility with the existing
    // RoomScene structure. The Tiled draw path in drawFloor() below
    // performs the actual layered rendering for rooms present in g_tiledRooms.
    scene.floor.assign(height,std::vector<TileRef>(width, {-1, -1}));
    scene.decor.assign(height,std::vector<TileRef>(width, {-1, -1}));

    // Read *this* map's own <tileset firstgid=.../> declarations instead of
    // assuming every room shares The_Camp_Ground.tmx's GID layout. This is
    // what makes single-tileset maps like Hut1.tmx (Dungeon_Tiles only,
    // firstgid=1) resolve correctly instead of being misread as Floors.
    const std::vector<std::pair<int,int>>& gidRanges = data.gidRanges;

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const int index = row * width + col;
            scene.floor[row][col] = tiledGidToTileRef(ground[index], gidRanges);
            if (decor[index] != 0) {
                scene.decor[row][col] = tiledGidToTileRef(decor[index], gidRanges);
            }
        }
    }
    loadTmxDoors(tmx, width * 16, height * 16, data.doors);
    std::cout << "[DEBUG] Room '" << roomName << "' loaded " << data.doors.size() << " door(s).\n";
    loadTmxProps(tmx, width * 16, height * 16, data.props);  

    std::cout << "[Tiled] Loaded " << path << " (" << width << "x" << height << ")\n";

    return true;
}

bool RoomSceneManager::getDoorAt(const std::string& roomName, float relX, float relY, TiledDoor& outDoor) const {
    auto tiledIt = g_tiledRooms.find(roomName);
    if (tiledIt == g_tiledRooms.end()) return false; // this room has no Tiled map / no doors
    for (const TiledDoor& d : tiledIt->second.doors) {
        if (relX >= d.x0 && relX <= d.x1 && relY >= d.y0 && relY <= d.y1) {
            outDoor = d;
            return true;
        }
    }
    return false;
}

void RoomSceneManager::buildLayouts(const World& world, int cols, int rows) {
    int floorsIdx  = tilesetIndex.count("floors")  ? tilesetIndex["floors"]  : -1;
    int wallsIdx   = tilesetIndex.count("walls")   ? tilesetIndex["walls"]   : -1;
    int waterIdx   = tilesetIndex.count("water")   ? tilesetIndex["water"]   : -1;
    int dungeonIdx = tilesetIndex.count("dungeon") ? tilesetIndex["dungeon"] : -1;

    for (Room* r : world.getRooms()) {
        std::string name = r->getName();

        // -------------------------------------------------------------
        // TILED ROOMS
        //
        // Any room whose name maps to an existing assets/maps/*.tmx file
        // (see tiledMapPathForRoom) loads from Tiled automatically — no
        // per-room code needed here. Rooms without a matching file fall
        // through to the manual/auto-generated layout below.
        // -------------------------------------------------------------
        {
            const std::string tiledPath = tiledMapPathForRoom(name);
            std::ifstream probe(tiledPath);
            if (probe.good()) {
                probe.close();
                RoomScene tiledScene;
                if (loadTiledRoom(tiledPath, name, tiledScene, floorsIdx, dungeonIdx, wallsIdx, waterIdx)) {
                    tiledScene.props = g_tiledRooms[name].props; 
                    rooms[name] = tiledScene;
                    std::cout << "[Tiled] Using Tiled map for " << name << "\n";
                    continue;
                }
                else {
                    std::cout << "[Tiled] No map found at '" << tiledPath << "' for room '" << name << "' — using manual/auto layout instead.\n";
                }
                std::cerr << "[Tiled] Failed to load " << tiledPath << ". Falling back to C++ layout.\n";
            }
        }

        // -------------------------------------------------------------
        // EXISTING MANUAL LAYOUT SYSTEM
        // -------------------------------------------------------------
        auto manualIt = g_manualLayouts.find(name);

        if (manualIt != g_manualLayouts.end()) {
            const ManualLayout& layout = manualIt->second;
            RoomScene scene;

            int layoutRows = (int)layout.rows.size();
            int layoutCols = layoutRows > 0 ? (int)layout.rows[0].size() : 0;

            scene.floor.assign(layoutRows,std::vector<TileRef>(layoutCols,{-1, -1}));

            for (int rr = 0; rr < layoutRows; rr++) {
                for (int cc = 0;cc < layoutCols &&cc < (int)layout.rows[rr].size();cc++) {
                    char ch = layout.rows[rr][cc];
                    auto legendIt =layout.legend.find(ch);

                    scene.floor[rr][cc] =(legendIt != layout.legend.end()) ? legendIt->second: TileRef{-1, -1};
                }
            }
            scene.decor.assign(layoutRows,std::vector<TileRef>(layoutCols,{-1, -1}));

            for (int rr = 0;rr < layoutRows &&rr < (int)layout.decorRows.size();rr++) {
                for (int cc = 0;cc < layoutCols &&cc < (int)layout.decorRows[rr].size();cc++) {
                    char ch =layout.decorRows[rr][cc];
                    auto it =layout.decorLegend.find(ch);
                    scene.decor[rr][cc] = (it != layout.decorLegend.end()) ? it->second: TileRef{-1, -1};
                }
            }
            scene.decorFeatures = layout.decorFeatures;
            rooms[name] = scene;
            continue;
        }

        // -------------------------------------------------------------
        // EXISTING AUTO-GENERATED FALLBACK
        // -------------------------------------------------------------
        int useFloorSet  = floorsIdx;
        int useFloorTile = FLOOR_TILE_INDEX;

        int useWallSet   = wallsIdx;
        int useWallTile  = WALL_TILE_INDEX;

        if (name == "The Lake") {
            useFloorSet  = waterIdx;
            useFloorTile = WATER_TILE_INDEX;
        }
        else if (name == "The Maze" || name.rfind("Maze", 0) == 0) {
            useFloorSet  = dungeonIdx;
            useFloorTile = DUNGEON_TILE_INDEX;

            useWallSet   = dungeonIdx;
            useWallTile  = DUNGEON_WALL_INDEX;
        }
        RoomScene scene;
        scene.floor.assign(rows,std::vector<TileRef>(cols,{useFloorSet, useFloorTile}));
        for (int c = 0; c < cols; c++) {
            scene.floor[0][c] = {useWallSet, useWallTile};
            scene.floor[rows - 1][c] = {useWallSet, useWallTile};
        }
        for (int rr = 0; rr < rows; rr++) {
            scene.floor[rr][0] = {useWallSet, useWallTile}; 
            scene.floor[rr][cols - 1] ={useWallSet, useWallTile};
        }
        rooms[name] = scene;
    }
    // Existing animated props.
    if (rooms.count("The Camp Ground")) {
        rooms["The Camp Ground"].props.push_back({"bonfire", 0.5f, 0.6f});
    }
    if (rooms.count("The Shed")) {
        rooms["The Shed"].props.push_back({"forge_iron", 0.5f, 0.55f});
    }
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
    const int tileDraw = (int)(16 * scale);
    // -------------------------------------------------------------
    // Tiled rooms
    //
    // Draw the actual Tiled layers instead of flattening them into one
    // TileRef per cell. This preserves the ground underneath transparent
    // wall/autotile pixels. Works for any room present in g_tiledRooms,
    // not just one hardcoded room.
    // -------------------------------------------------------------
    auto tiledIt = g_tiledRooms.find(roomName);
    if (tiledIt != g_tiledRooms.end() && tiledIt->second.loaded && tiledIt->second.width > 0 && tiledIt->second.height > 0){
        const TiledRoomData& td = tiledIt->second;

        auto drawTiledLayer = [&](const std::vector<int>& layer){
            if ((int)layer.size() != td.width * td.height) {
                return;
            }

            for (int row = 0; row < td.height; ++row) {
                for (int col = 0; col < td.width; ++col) {
                    const int gid = layer[row * td.width + col];
                    if (gid == 0) continue;

                    const TileRef t = tiledGidToTileRef( gid, td.gidRanges);
                
                    if (t.tilesetId < 0 || t.tilesetId >= (int)tilesets.size() || t.tileIndex < 0) {
                        continue;
                    }

                    const int x = originX + col * tileDraw;
                    const int y = originY + row * tileDraw;

                    if (tilesets[t.tilesetId].isLoaded()) {
                        tilesets[t.tilesetId].drawTile(t.tileIndex,x,y,scale);
                    }

                    else {
                        DrawRectangle(x,y,tileDraw,tileDraw,TILESET_COLORS[t.tilesetId]);
                    }
                }
            }
        };

        // Match the layer concept from Tiled:
        // base -> overlays. Transparent wall pixels now reveal ground.
        drawTiledLayer(td.ground);
        drawTiledLayer(td.water);
        drawTiledLayer(td.walls);

        return;
    }

    // -------------------------------------------------------------
    // Existing manual-room renderer — unchanged.
    // -------------------------------------------------------------
    const RoomScene& scene = it->second;

    for (size_t r = 0; r < scene.floor.size(); r++) {
        for (size_t c = 0; c < scene.floor[r].size(); c++) {
            const TileRef& t = scene.floor[r][c];
            if (t.tilesetId < 0 || t.tilesetId >= (int)tilesets.size()) {
                continue;
            }

            if (tilesets[t.tilesetId].isLoaded()) {
                tilesets[t.tilesetId].drawTile(t.tileIndex,originX + (int)c * tileDraw,originY + (int)r * tileDraw,scale);
            } 
            else {
                DrawRectangle(originX + (int)c * tileDraw,originY + (int)r * tileDraw,tileDraw,tileDraw,TILESET_COLORS[t.tilesetId]);
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
            tilesets[t.tilesetId].drawTile(t.tileIndex,originX + c * tileDraw, originY + r * tileDraw, scale);
        }
    }
}
void RoomSceneManager::drawDecorFeatures(const std::string& roomName, int originX, int originY, float scale) const {
    auto it = rooms.find(roomName);
    if (it == rooms.end()) return;

    int tileDraw = (int)(16 * scale);
    for (const DecorFeature& f : it->second.decorFeatures) {
        if (f.tilesetId < 0 || f.tilesetId >= (int)tilesets.size()) continue;
        tilesets[f.tilesetId].drawRegion(f.sheetCol, f.sheetRow, f.cellsWide, f.cellsHigh, originX + f.gridCol * tileDraw, originY + f.gridRow * tileDraw, scale);
    }
}
void RoomSceneManager::drawProps(const std::string& roomName, int originX, int originY, int viewportW, int viewportH, float scale) const {
    auto it = rooms.find(roomName);
    if (it == rooms.end()) return;

    for (const PropInstance& p : it->second.props) {
        auto animIt = propAnimators.find(p.clipName);
        if (animIt == propAnimators.end()) continue;
        int x = originX + (int)(p.relX * viewportW);
        int y = originY + (int)(p.relY * viewportH);
        float propScale = scale;

        if (p.clipName == "bonfire") {
            propScale *= 1.5f;
        }

        animIt->second.draw(x, y, propScale);    
    }
}
void RoomSceneManager::drawNpcs(const std::vector<NPC*>& npcsInRoom, int originX, int originY, int viewportW, int viewportH, float scale) const {
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