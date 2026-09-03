#include "roomscene.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>


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
static const int DUNGEON_WALL_INDEX = 2;   // Dungeon_Tiles.png row 0, col 2  — solid, tiles clean


// Per-room Tiled map cache. Keyed by room name, so any number of rooms can
// each load their own .tmx independently — this used to be a single set of
// globals hardcoded to "The Camp Ground", meaning loading a second Tiled
// room would silently overwrite the first one's data (broken collision,
// broken doors, blank rendering for whichever room loaded first).
struct TiledRoomData {
    std::vector<int> ground, walls, water, collision;
    int width = 0, height = 0;
    std::vector<TiledDoor> doors;
    std::vector<PropInstance> props;
    bool loaded = false;
    std::vector<std::pair<int,int>> gidRanges;
};
static std::unordered_map<std::string, TiledRoomData> g_tiledRooms;

// Maps each room's CURRENT display name to its actual .tmx filename on
// disk. Deliberately explicit (not derived from the room name) because
// room names get reskinned/renamed independently of the asset files —
// see the Glade->Hollow, Cage->Chute, Walls->Ramparts, Maze->Labyrinth
// rename, which broke the old space->underscore derivation below.
// Any room NOT listed here still falls back to the old derivation, so
// new rooms don't need an entry unless their display name diverges from
// their filename.
static const std::unordered_map<std::string, std::string> roomTmxOverrides = {
    { "The Hollow",    "assets/maps/The_Glade.tmx"  },
    { "The Chute",     "assets/maps/The_Cage.tmx"   },
    { "The Ramparts",  "assets/maps/The_Walls.tmx"  },
    { "The Labyrinth", "assets/maps/The_Maze.tmx"   },
};

static std::string tiledMapPathForRoom(const std::string& roomName) {
    auto it = roomTmxOverrides.find(roomName);
    if (it != roomTmxOverrides.end()) return it->second;

    std::string fname = roomName;
    for (char& c : fname) if (c == ' ') c = '_';
    return "assets/maps/" + fname + ".tmx";
}


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

// defineManualLayouts() — UNUSED as of switching to Tiled.
//
// Camp Ground, Cage, and Shed all moved to their own assets/maps/*.tmx and
// load through the Tiled path in buildLayouts(), which `continue`s before
// this system is ever consulted. The manual ASCII-layout system itself
// (the ManualLayout struct, g_manualLayouts, and the lookup branches in
// buildLayouts()/isTileBlocked() that used to read it) has been removed
// entirely — this function is kept as a no-op purely because it's declared
// in roomscene.h and still called from loadTilesets(); leaving it in place
// avoids a header change for a function that costs nothing to keep empty.
void RoomSceneManager::defineManualLayouts() {
    // Intentionally empty.
}

// Coordinates just outside the viewport are intentionally allowed so the
// room-exit transition logic in main.cpp can trigger the door/exit change.
// If we reject those as wall collisions here, the player never reaches the
// >1.0 checks that call "go east" / "go south".
bool RoomSceneManager::isTileBlocked(const std::string& roomName,
                                      float relX, float relY) const {
    if (relX < 0.0f || relX > 1.0f || relY < 0.0f || relY > 1.0f) {
        return false;
    }

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

    // Rooms with no Tiled map (the Maze1-14 auto-generated fallback rooms,
    // for now) have no collision data — nothing blocks movement in them.
    return false;
}

void RoomSceneManager::loadTilesets(const std::string& assetDir) {
    registerTileset("floors",  assetDir + "/tiles/Floors_Tiles.png");
    registerTileset("walls",   assetDir + "/tiles/Wall_Tiles.png");
    registerTileset("water",   assetDir + "/tiles/Water_tiles.png");
    registerTileset("dungeon", assetDir + "/tiles/Dungeon_Tiles.png");

    registerTileset("interior_walls", assetDir + "/tiles/Interior_Walls_01.png");
    registerTileset("interior_props", assetDir + "/tiles/Interior_Props_01.png");
    registerTileset("props",          assetDir + "/tiles/Props.png");
    registerTileset("roofs",          assetDir + "/tiles/Roofs.png");
    registerTileset("wall_variations",assetDir + "/tiles/Wall_Variations.png");
    registerTileset("shadows",        assetDir + "/tiles/Shadows.png");

    defineManualLayouts(); // no-op — kept only because loadTilesets() has always called it
}

void RoomSceneManager::loadNpcPortraits(const std::string& assetDir) {
    // Map keys are each NPC's CURRENT display name (what getName() returns
    // and what getPortrait() is looked up by) — file paths stay pointed at
    // the original asset folders/filenames, since those are just internal
    // disk paths and never shown to the player.
    npcPortraits["Isolde"]     = LoadTexture((assetDir + "/npc/Alby/Alby_face.png").c_str());
    npcPortraits["Brecht"]     = LoadTexture((assetDir + "/npc/Gally/Gally_face.png").c_str());
    npcPortraits["Rooke"]      = LoadTexture((assetDir + "/npc/Minho/Minho_face.png").c_str());
    npcPortraits["Wren"]       = LoadTexture((assetDir + "/npc/Terrisa/terrisa_face.png").c_str());
    npcPortraits["Aldric"]     = LoadTexture((assetDir + "/npc/Newt/Newt_face.png").c_str());
    npcPortraits["Doc Marrow"] = LoadTexture((assetDir + "/npc/Pete/Pete_face.png").c_str());
}

Texture2D RoomSceneManager::getPortrait(const std::string& name) const {
    auto it = npcPortraits.find(name);
    if (it != npcPortraits.end()) return it->second;
    return Texture2D{};   // id=0 signals "no portrait" — caller checks before drawing
}

void RoomSceneManager::loadNpcSprites(const std::string& assetDir) {
    // Aldric (was Newt) — map key is the current display name; file paths
    // are untouched, since they're internal disk paths, not player-facing.
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Newt/newt_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Newt/newt_walk.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Newt/newt_hurt.png", 3, 0.15f, 48, 48, 3);
        npcAnimators["Aldric"] = std::move(a);
    }
    // Brecht (was Gally)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Gally/Gally_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Gally/Gally_walking.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Gally/Gally_Hurt.png",    3, 0.15f, 48, 48, 3);
        npcAnimators["Brecht"] = std::move(a);
    }
    // Rooke (was Minho)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Minho/Minho_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Minho/Minho_walk.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Minho/Minho_hurt.png", 3, 0.15f, 48, 48, 3);
        npcAnimators["Rooke"] = std::move(a);
    }
    // Wren (was Terrisa)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Terrisa/terissa_walk.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Terrisa/terissa_walk.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Terrisa/terissa_hurt.png", 3, 0.15f, 48, 48, 3);
        npcAnimators["Wren"] = std::move(a);
    }
    // Isolde (was Alby)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Alby/Alby_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Alby/Alby_walking.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Alby/Alby_hurt.png",    3, 0.15f, 48, 48, 3);
        npcAnimators["Isolde"] = std::move(a);
    }
    // Doc Marrow (was Pete)
    {
        StripAnimator a;
        a.addClip("idle", assetDir + "/npc/Pete/Pete_walking.png", 3, 0.15f, 48, 48, 3);
        a.addClip("run",  assetDir + "/npc/Pete/Pete_walking.png", 3, 0.10f, 48, 48, 3);
        a.addClip("hurt", assetDir + "/npc/Pete/Pete_hurt.png",    3, 0.15f, 48, 48, 3);
        npcAnimators["Doc Marrow"] = std::move(a);
    }
}
void RoomSceneManager::loadProps(const std::string& assetDir) {
    {
        StripAnimator a;
        a.addClip("burn",
                  assetDir + "/Props/Bonfire_01-Sheet.png",
                  4, 0.15f, 32, 32);
        propAnimators["bonfire"] = std::move(a);
    }

    {
        StripAnimator a;
        a.addClip("burn",
                  assetDir + "/Props/Iron_01-Sheet.png",
                  2, 0.30f, 32, 96);
        propAnimators["forge_iron"] = std::move(a);
    }
}

void RoomSceneManager::loadMonsterSprites(const std::string& assetDir) {
    // Build a fresh animator per Warden name — can't share one StripAnimator
    // across map entries since it's move-only (each move empties the source).
    for (const std::string n : {"The First Warden", "Warden 1", "Warden 2",
                                  "Warden 3", "Warden 5", "Warden 7", "Warden 8"}) {
        StripAnimator warden;
        warden.addClip("idle", assetDir + "/Mobs/Orc Crew/Orc - Warrior/Idle/Idle-Sheet.png", 4, 0.15f, 32, 32);
        warden.addClip("run",  assetDir + "/Mobs/Orc Crew/Orc - Warrior/Run/Run-Sheet.png",  6, 0.10f, 64, 64);
        npcAnimators[n] = std::move(warden);
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


// Reads every <tileset firstgid="N" source="....tsx"/> declaration out of a
// TMX file and maps each one, by filename, to a runtime tileset index using
// whatever tilesets have been registered via registerTileset(). Matching is
// substring-based against each registered tileset's own name/path so any
// number of sheets can be added without touching this function again.
static std::vector<std::pair<int,int>> parseTmxTilesetRanges(
        const std::string& tmx,
        const std::unordered_map<std::string,int>& tilesetIndex) {

    std::vector<std::pair<int,int>> ranges;

    std::regex tsRegex(
        R"RX(<tileset\s+firstgid="(\d+)"\s+source="([^"]*)")RX",
        std::regex_constants::icase
    );

    for (std::sregex_iterator it(tmx.begin(), tmx.end(), tsRegex), end;
         it != end; ++it) {

        int firstgid = std::stoi((*it)[1].str());
        std::string source = (*it)[2].str();

        // Get just the filename.
        std::string filename = source;
        size_t slash = filename.find_last_of("/\\");
        if (slash != std::string::npos) {
            filename = filename.substr(slash + 1);
        }

        // Remove extension.
        size_t dot = filename.find_last_of('.');
        std::string stem =
            (dot != std::string::npos)
                ? filename.substr(0, dot)
                : filename;

        // Lowercase for matching.
        std::string stemLower = stem;
        std::transform(
            stemLower.begin(),
            stemLower.end(),
            stemLower.begin(),
            [](unsigned char c) {
                return (char)std::tolower(c);
            }
        );

        int idx = -1;
        size_t bestLen = 0;

        // Match against the registered runtime tileset names.
        for (const auto& kv : tilesetIndex) {

            std::string keyLower = kv.first;

            std::transform(
                keyLower.begin(),
                keyLower.end(),
                keyLower.begin(),
                [](unsigned char c) {
                    return (char)std::tolower(c);
                }
            );

            // Normal match.
            if (stemLower.find(keyLower) != std::string::npos &&
                keyLower.size() > bestLen) {

                idx = kv.second;
                bestLen = keyLower.size();
            }

            // Special case:
            // runtime name is "walls", but the actual tileset is
            // "Wall_Tiles".
            //
            // "wall_tiles" does not contain "walls", so explicitly
            // treat Wall_Tiles as the registered "walls" tileset.
            if (keyLower == "walls" &&
                (stemLower == "wall_tiles" ||
                 stemLower.find("wall_tiles") != std::string::npos)) {

                idx = kv.second;
                bestLen = keyLower.size();
            }
        }

        if (idx != -1) {
            ranges.push_back({firstgid, idx});

            std::cout
                << "[Tiled] Tileset '" << source
                << "' -> runtime tileset " << idx
                << " (firstgid " << firstgid << ")\n";
        }
        else {
            std::cerr
                << "[Tiled] Unrecognized tileset source '"
                << source
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
// Parses <objectgroup name="props"> ... <object .../> ... </objectgroup>.
// Point/rect objects placed in Tiled under a "props" layer. Each object's
// prop name — which clip in RoomSceneManager::loadProps() to draw (e.g.
// "bonfire", "forge_iron") — can come from either place Tiled might put it:
//   1. name="bonfire" directly on the <object> tag, OR
//   2. a nested custom property: <property name="Name" value="bonfire"/>
// (2) is what Tiled produces if you add a custom "Name" property in the
// object's Properties panel instead of using the object's own Name field —
// easy to do by accident, and what The_Camp_Ground.tmx actually has.
// Position is stored as a 0..1 fraction of the map so any room size works
// the same way doors already do; when width/height are present we center
// on the object's rect rather than using its top-left corner.
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

    // Capture id/name(optional)/x/y/width(optional)/height(optional), then
    // the object's inner body (group 6) so we can also look for a nested
    // <property name="Name" value=".."/> when the tag itself has no name.
    // Self-closing (<object .../>) and open/close (<object ..>...</object>)
    // forms are both handled.
    std::regex objRegex(
        R"RX(<object\s+id="\d+"(?:\s+name="([^"]*)")?\s+x="([-\d.]+)"\s+y="([-\d.]+)"(?:\s+width="([-\d.]+)")?(?:\s+height="([-\d.]+)")?\s*(?:/>|>([\s\S]*?)</object>))RX");

    std::regex namePropRegex(
        R"RX(<property\s+name="Name"\s+value="([^"]*)"\s*/>)RX",
        std::regex_constants::icase);

    for (std::sregex_iterator it(block.begin(), block.end(), objRegex), end; it != end; ++it) {
        std::smatch m = *it;

        std::string clipName = m[1].matched ? m[1].str() : "";
        if (clipName.empty() && m[6].matched) {
            std::smatch pm;
            std::string body = m[6].str();
            if (std::regex_search(body, pm, namePropRegex)) {
                clipName = pm[1].str();
            }
        }
        if (clipName.empty()) continue;

        float x = std::stof(m[2].str());
        float y = std::stof(m[3].str());
        float w = m[4].matched ? std::stof(m[4].str()) : 0.0f;
        float h = m[5].matched ? std::stof(m[5].str()) : 0.0f;

        PropInstance p;
        p.clipName = clipName;
        p.relX = (x + w * 0.5f) / (float)mapWidthPx;
        p.relY = (y + h * 0.5f) / (float)mapHeightPx;
        out.push_back(p);
    }
}
static std::vector<std::pair<int,int>> parseTmxTilesetRanges(
    const std::string& tmx,
    const std::unordered_map<std::string,int>& tilesetIndex);

static bool loadTiledRoom(const std::string& path, const std::string& roomName,
                           RoomScene& scene,const std::unordered_map<std::string,int>& tilesetIndex) {

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
    data.gidRanges = parseTmxTilesetRanges(tmx, tilesetIndex);
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
        // through to the auto-generated layout below.
        // -------------------------------------------------------------
        {
            const std::string tiledPath = tiledMapPathForRoom(name);
            std::ifstream probe(tiledPath);
            if (probe.good()) {
                probe.close();
                RoomScene tiledScene;
                if (loadTiledRoom(tiledPath, name, tiledScene, tilesetIndex)) {
                    tiledScene.props = g_tiledRooms[name].props;
                    rooms[name] = tiledScene;
                    std::cout << "[Tiled] Using Tiled map for " << name << "\n";
                    continue;
                }
                else {
                    std::cout << "[Tiled] No map found at '" << tiledPath << "' for room '" << name << "' — using auto-generated layout instead.\n";
                }
                std::cerr << "[Tiled] Failed to load " << tiledPath << ". Falling back to C++ layout.\n";
            }
        }

        // -------------------------------------------------------------
        // AUTO-GENERATED FALLBACK
        //
        // Used for any room with no matching .tmx — currently the
        // procedurally-created Maze1-14 sub-rooms.
        // -------------------------------------------------------------
        int useFloorSet  = floorsIdx;
        int useFloorTile = FLOOR_TILE_INDEX;

        int useWallSet   = wallsIdx;
        int useWallTile  = WALL_TILE_INDEX;

        if (name == "The Lake") {
            useFloorSet  = waterIdx;
            useFloorTile = WATER_TILE_INDEX;
        }
        else if (name == "The Labyrinth" || name.rfind("Maze", 0) == 0) {
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
                    Color fallback = (t.tilesetId < 4) ? TILESET_COLORS[t.tilesetId] : Color{200, 0, 200, 255};
                    DrawRectangle(x, y, tileDraw, tileDraw, fallback);
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
    // Auto-generated-fallback room renderer.
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

        // 5. Stash this frame's position on the NPC itself (relative 0..1,
        // same convention as doors/props) so proximity checks elsewhere
        // (GameLoop::checkNpcProximity) always match what's on screen.
        float relX = (viewportW > 0) ? (float)(x - originX) / (float)viewportW : 0.5f;
        float relY = (viewportH > 0) ? (float)(y - originY) / (float)viewportH : 0.5f;
        npcsInRoom[i]->setPosition(relX, relY);
    }
}