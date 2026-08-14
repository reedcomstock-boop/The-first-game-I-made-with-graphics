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


// Tiled Camp Ground layer cache.
// These are used only when drawing "The Camp Ground"; all other rooms keep
// using the existing RoomScene floor/decor arrays unchanged.
static std::vector<int> g_tiledGround;
static std::vector<int> g_tiledWalls;
static std::vector<int> g_tiledWater;
static int g_tiledWidth = 0;
static int g_tiledHeight = 0;
static bool g_tiledCampGroundLoaded = false;
 
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

void RoomSceneManager::loadTilesets(const std::string& assetDir) {
    registerTileset("floors",  assetDir + "/tiles/Floors_Tiles.png");
    registerTileset("walls",   assetDir + "/tiles/Wall_Tiles.png");
    registerTileset("water",   assetDir + "/tiles/Water_tiles.png");
    registerTileset("dungeon", assetDir + "/tiles/Dungeon_Tiles.png");
    defineManualLayouts(); // must come after tilesets are registered — needs their indices
}

void RoomSceneManager::loadNpcPortraits(const std::string& assetDir) {
    npcPortraits["Alby"] = LoadTexture((assetDir + "/npc/Alby/Alby_face.png").c_str());
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

static bool loadTmxLayerCSV(
    const std::string& tmx,
    const std::string& layerName,
    int expectedWidth,
    int expectedHeight,
    std::vector<int>& out)
{
    std::regex layerRegex(
        "<layer[^>]*name=\"" + layerName +
        "\"[^>]*>[\\s\\S]*?<data[^>]*encoding=\"csv\"[^>]*>([\\s\\S]*?)</data>",
        std::regex_constants::icase
    );

    std::smatch match;

    if (!std::regex_search(tmx, match, layerRegex)) {
        return false;
    }

    std::string csv = match[1].str();
    std::stringstream ss(csv);
    std::string value;

    out.clear();

    while (std::getline(ss, value, ',')) {
        value.erase(
            std::remove_if(
                value.begin(),
                value.end(),
                [](unsigned char c) {
                    return std::isspace(c);
                }
            ),
            value.end()
        );

        if (!value.empty()) {
            out.push_back(std::stoi(value));
        }
    }

    return (int)out.size() == expectedWidth * expectedHeight;
}


// Convert a Tiled global tile ID into the local tile index used by TileSet.
//
// The current The_Camp_ground.tmx has:
//
//   Floors_Tiles  firstgid = 1
//   Dungeon_Tiles firstgid = 651
//   Wall_Tiles    firstgid = 1276
//
// The four runtime tilesets are registered in this order:
//
//   floors = 0
//   walls  = 1
//   water  = 2
//   dungeon = 3
//
// Tiled's high bits are reserved for horizontal/vertical/diagonal flips.
// We do not currently use flipped tiles, but mask those bits so a flipped
// tile will not turn into an invalid tile index.
static TileRef tiledGidToTileRef(
    int gid,
    int floorsIdx,
    int dungeonIdx,
    int wallsIdx)
{
    // Empty cell.
    if (gid <= 0) {
        return {-1, -1};
    }

    // Remove Tiled flip flags.
    const int TILED_GID_MASK = 0x1FFFFFFF;
    gid &= TILED_GID_MASK;

    if (gid >= 1 && gid < 651) {
        return {floorsIdx, gid - 1};
    }

    if (gid >= 651 && gid < 1276) {
        return {dungeonIdx, gid - 651};
    }

    if (gid >= 1276) {
        return {wallsIdx, gid - 1276};
    }

    return {-1, -1};
}


static bool loadTiledCampGround(
    const std::string& path,
    RoomScene& scene,
    int floorsIdx,
    int dungeonIdx,
    int wallsIdx)
{
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr
            << "[Tiled] Could not open map: "
            << path << "\n";

        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string tmx = buffer.str();

    // Read the map dimensions directly from the TMX file.  This means you
    // can resize the map in Tiled without changing C++ code.
    std::regex mapRegex(
        R"TMX(<map\b[^>]*\bwidth="([0-9]+)"[^>]*\bheight="([0-9]+)")TMX",
        std::regex_constants::icase
    );

    std::smatch mapMatch;
    if (!std::regex_search(tmx, mapMatch, mapRegex)) {
        std::cerr << "[Tiled] Could not read map dimensions.\n";
        return false;
    }

    const int width = std::stoi(mapMatch[1].str());
    const int height = std::stoi(mapMatch[2].str());

    if (width <= 0 || height <= 0) {
        std::cerr << "[Tiled] Invalid map dimensions: "
                  << width << "x" << height << "\n";
        return false;
    }

    std::vector<int> ground;
    std::vector<int> walls;
    std::vector<int> water;
    std::vector<int> decor;

    if (!loadTmxLayerCSV(
            tmx, "ground", width, height, ground))
    {
        std::cerr
            << "[Tiled] Could not read ground layer.\n";

        return false;
    }

    if (!loadTmxLayerCSV(
            tmx, "walls", width, height, walls))
    {
        std::cerr
            << "[Tiled] Could not read walls layer.\n";

        return false;
    }

    // These layers are optional for this first test.
    loadTmxLayerCSV(
        tmx, "water", width, height, water);

    loadTmxLayerCSV(
        tmx, "decor", width, height, decor);

    // RoomScene's existing renderer expects the floor array to contain the
    // visible base tile for every cell.  For this first integration pass,
    // we flatten the visible Tiled ground/walls/water layers into that array.
    scene.floor.assign(
        height,
        std::vector<TileRef>(
            width,
            {-1, -1}
        )
    );

    scene.decor.assign(
        height,
        std::vector<TileRef>(
            width,
            {-1, -1}
        )
    );

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
    g_tiledGround = ground;
    g_tiledWalls  = walls;
    g_tiledWater  = water;
    g_tiledWidth  = width;
    g_tiledHeight = height;
    g_tiledCampGroundLoaded = true;

    // Keep these arrays populated for compatibility with the existing
    // RoomScene structure. The Camp Ground's drawFloor() below performs
    // the actual layered rendering.
    scene.floor.assign(
        height,
        std::vector<TileRef>(width, {-1, -1})
    );

    scene.decor.assign(
        height,
        std::vector<TileRef>(width, {-1, -1})
    );

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const int index = row * width + col;

            scene.floor[row][col] =
                tiledGidToTileRef(
                    ground[index],
                    floorsIdx,
                    dungeonIdx,
                    wallsIdx
                );

            if (decor[index] != 0) {
                scene.decor[row][col] =
                    tiledGidToTileRef(
                        decor[index],
                        floorsIdx,
                        dungeonIdx,
                        wallsIdx
                    );
            }
        }
    }

    std::cout
        << "[Tiled] Loaded "
        << path
        << " (" << width << "x" << height << ")\n";

    return true;
}


void RoomSceneManager::buildLayouts(const World& world, int cols, int rows) {
    int floorsIdx  = tilesetIndex.count("floors")  ? tilesetIndex["floors"]  : -1;
    int wallsIdx   = tilesetIndex.count("walls")   ? tilesetIndex["walls"]   : -1;
    int waterIdx   = tilesetIndex.count("water")   ? tilesetIndex["water"]   : -1;
    int dungeonIdx = tilesetIndex.count("dungeon") ? tilesetIndex["dungeon"] : -1;

    for (Room* r : world.getRooms()) {
        std::string name = r->getName();

        // -------------------------------------------------------------
        // TILED TEST
        //
        // Only The Camp Ground uses the Tiled map for now.
        // Every other room continues using the existing C++ system.
        // -------------------------------------------------------------
        if (name == "The Camp Ground") {
            RoomScene tiledScene;

            const std::string tiledPath =
                "assets/maps/The_Camp_ground.tmx";

            if (loadTiledCampGround(
                    tiledPath,
                    tiledScene,
                    floorsIdx,
                    dungeonIdx,
                    wallsIdx))
            {
                rooms[name] = tiledScene;

                std::cout
                    << "[Tiled] Using Tiled map for "
                    << name << "\n";

                continue;
            }

            std::cerr
                << "[Tiled] Failed to load "
                << tiledPath
                << ". Falling back to C++ layout.\n";
        }

        // -------------------------------------------------------------
        // EXISTING MANUAL LAYOUT SYSTEM
        // -------------------------------------------------------------
        auto manualIt = g_manualLayouts.find(name);

        if (manualIt != g_manualLayouts.end()) {
            const ManualLayout& layout = manualIt->second;
            RoomScene scene;

            int layoutRows = (int)layout.rows.size();
            int layoutCols =
                layoutRows > 0
                    ? (int)layout.rows[0].size()
                    : 0;

            scene.floor.assign(
                layoutRows,
                std::vector<TileRef>(
                    layoutCols,
                    {-1, -1}
                )
            );

            for (int rr = 0; rr < layoutRows; rr++) {
                for (
                    int cc = 0;
                    cc < layoutCols &&
                    cc < (int)layout.rows[rr].size();
                    cc++
                ) {
                    char ch = layout.rows[rr][cc];

                    auto legendIt =
                        layout.legend.find(ch);

                    scene.floor[rr][cc] =
                        (legendIt != layout.legend.end())
                            ? legendIt->second
                            : TileRef{-1, -1};
                }
            }

            scene.decor.assign(
                layoutRows,
                std::vector<TileRef>(
                    layoutCols,
                    {-1, -1}
                )
            );

            for (
                int rr = 0;
                rr < layoutRows &&
                rr < (int)layout.decorRows.size();
                rr++
            ) {
                for (
                    int cc = 0;
                    cc < layoutCols &&
                    cc < (int)layout.decorRows[rr].size();
                    cc++
                ) {
                    char ch =
                        layout.decorRows[rr][cc];

                    auto it =
                        layout.decorLegend.find(ch);

                    scene.decor[rr][cc] =
                        (it != layout.decorLegend.end())
                            ? it->second
                            : TileRef{-1, -1};
                }
            }

            scene.decorFeatures =
                layout.decorFeatures;

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
        else if (
            name == "The Maze" ||
            name.rfind("Maze", 0) == 0
        ) {
            useFloorSet  = dungeonIdx;
            useFloorTile = DUNGEON_TILE_INDEX;

            useWallSet   = dungeonIdx;
            useWallTile  = DUNGEON_WALL_INDEX;
        }

        RoomScene scene;

        scene.floor.assign(
            rows,
            std::vector<TileRef>(
                cols,
                {useFloorSet, useFloorTile}
            )
        );

        for (int c = 0; c < cols; c++) {
            scene.floor[0][c] =
                {useWallSet, useWallTile};

            scene.floor[rows - 1][c] =
                {useWallSet, useWallTile};
        }

        for (int rr = 0; rr < rows; rr++) {
            scene.floor[rr][0] =
                {useWallSet, useWallTile};

            scene.floor[rr][cols - 1] =
                {useWallSet, useWallTile};
        }

        rooms[name] = scene;
    }

    // Existing animated props.
    if (rooms.count("The Camp Ground")) {
        rooms["The Camp Ground"]
            .props.push_back(
                {"bonfire", 0.5f, 0.6f}
            );
    }

    if (rooms.count("The Shed")) {
        rooms["The Shed"]
            .props.push_back(
                {"forge_iron", 0.5f, 0.55f}
            );
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
    // Tiled Camp Ground
    //
    // Draw the actual Tiled layers instead of flattening them into one
    // TileRef per cell. This preserves the ground underneath transparent
    // wall/autotile pixels.
    // -------------------------------------------------------------
    if (roomName == "The Camp Ground" &&
        g_tiledCampGroundLoaded &&
        g_tiledWidth > 0 &&
        g_tiledHeight > 0)
    {
        const int floorsIdx =
            tilesetIndex.count("floors")
                ? tilesetIndex.at("floors") : -1;
        const int dungeonIdx =
            tilesetIndex.count("dungeon")
                ? tilesetIndex.at("dungeon") : -1;
        const int wallsIdx =
            tilesetIndex.count("walls")
                ? tilesetIndex.at("walls") : -1;

        auto drawTiledLayer =
            [&](const std::vector<int>& layer)
        {
            if ((int)layer.size() != g_tiledWidth * g_tiledHeight) {
                return;
            }

            for (int row = 0; row < g_tiledHeight; ++row) {
                for (int col = 0; col < g_tiledWidth; ++col) {
                    const int gid =
                        layer[row * g_tiledWidth + col];

                    if (gid == 0) continue;

                    const TileRef t =
                        tiledGidToTileRef(
                            gid,
                            floorsIdx,
                            dungeonIdx,
                            wallsIdx
                        );

                    if (t.tilesetId < 0 ||
                        t.tilesetId >= (int)tilesets.size() ||
                        t.tileIndex < 0) {
                        continue;
                    }

                    const int x = originX + col * tileDraw;
                    const int y = originY + row * tileDraw;

                    if (tilesets[t.tilesetId].isLoaded()) {
                        tilesets[t.tilesetId].drawTile(
                            t.tileIndex,
                            x,
                            y,
                            scale
                        );
                    } else {
                        DrawRectangle(
                            x,
                            y,
                            tileDraw,
                            tileDraw,
                            TILESET_COLORS[t.tilesetId]
                        );
                    }
                }
            }
        };

        // Match the layer concept from Tiled:
        // base -> overlays. Transparent wall pixels now reveal ground.
        drawTiledLayer(g_tiledGround);
        drawTiledLayer(g_tiledWater);
        drawTiledLayer(g_tiledWalls);
        return;
    }

    // -------------------------------------------------------------
    // Existing manual-room renderer — unchanged.
    // -------------------------------------------------------------
    const RoomScene& scene = it->second;

    for (size_t r = 0; r < scene.floor.size(); r++) {
        for (size_t c = 0; c < scene.floor[r].size(); c++) {
            const TileRef& t = scene.floor[r][c];
            if (t.tilesetId < 0 ||
                t.tilesetId >= (int)tilesets.size()) {
                continue;
            }

            if (tilesets[t.tilesetId].isLoaded()) {
                tilesets[t.tilesetId].drawTile(
                    t.tileIndex,
                    originX + (int)c * tileDraw,
                    originY + (int)r * tileDraw,
                    scale
                );
            } else {
                DrawRectangle(
                    originX + (int)c * tileDraw,
                    originY + (int)r * tileDraw,
                    tileDraw,
                    tileDraw,
                    TILESET_COLORS[t.tilesetId]
                );
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
        float propScale = scale;

        if (p.clipName == "bonfire") {
            propScale *= 1.5f;
        }

        animIt->second.draw(x, y, propScale);    }
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