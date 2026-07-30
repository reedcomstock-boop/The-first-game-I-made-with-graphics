#include "graphics.h"
#include "raylib.h"
#include "roomscene.h"
#include <string>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <array>

// -----------------------------------------------------------------------
// Layout — computed as a percentage of the CURRENT window size each frame,
// using BASE_SW x BASE_SH (the original fixed 800x600 layout) as the
// reference the percentages were derived from. At 800x600 nothing changes;
// resizing the window scales every panel/font with it.
//
// NOTE: the animated scene view (tile floor, NPC sprites, props — drawn via
// g_scene.draw*()) keeps its original fixed tile size regardless of window
// size. Its column/row count is baked in at load time (SCENE_COLS/ROWS
// below), so on a bigger window you'll see more black margin around the
// scene rather than the tiles themselves growing.
// -----------------------------------------------------------------------
static const int BASE_SW = 800;
static const int BASE_SH = 600;
static const int BASE_PAD = 16;

static inline float SW() { return (float)GetScreenWidth(); }
static inline float SH() { return (float)GetScreenHeight(); }
static inline float SX() { return SW() / (float)BASE_SW; }  // width scale factor
static inline float SY() { return SH() / (float)BASE_SH; }  // height scale factor

static inline float PAD()      { return BASE_PAD   * SX(); }
static inline int   LINE_H()   { return (int)(22   * SY()); }
static inline int   FS()       { return (int)(16   * SY()); }   // base font size
static inline int   FS_TITLE() { return (int)(22   * SY()); }
static inline int   FS_SMALL() { return (int)(13   * SY()); }

// Panel rects — percentages of current screen size
static inline Rectangle ROOM_PANEL() {
    return { PAD(), PAD(), SW() - PAD()*2.0f - 160, (BASE_SH - 200) * SY() };
}
static inline Rectangle MINIMAP_PANEL() {
    return { 624 * SX(), 16 * SY(), 160 * SX(), 200 * SY() };
}
static inline Rectangle HUD_PANEL() {
    Rectangle map = MINIMAP_PANEL();
    return { map.x, map.y + map.height + 8 * SY(), map.width, 140 * SY() };
}
static inline Rectangle INPUT_PANEL() {
    return { PAD(), (BASE_SH - 75) * SY(), SW() - PAD()*2.0f, 50 * SY() };
}

// -----------------------------------------------------------------------
// Scene view (tile background + NPCs + animated props)
// Expects an "assets" folder next to the binary: assets/tiles, assets/npc, assets/props
// -----------------------------------------------------------------------
static const char* ASSET_DIR   = "assets";
static const float TILE_SCALE  = 2.0f;
static const int   TILE_DRAW   = 32;       // 16px source tile * TILE_SCALE
static const int   SCENE_H     = 150;      // height of the scene viewport inside ROOM_PANEL (base pixels, scaled by SY() at draw time)
static const int   SCENE_COLS  = (int)(BASE_SW - BASE_PAD*2 - BASE_PAD*2) / TILE_DRAW;
static const int   SCENE_ROWS  = SCENE_H / TILE_DRAW;

static RoomSceneManager g_scene;
static bool g_sceneLoaded = false;

static void ensureSceneLoaded(const World& world) {
    if (g_sceneLoaded) return;
    g_scene.loadTilesets(ASSET_DIR);
    g_scene.loadNpcSprites(ASSET_DIR);
    g_scene.loadMonsterSprites(ASSET_DIR);
    g_scene.loadProps(ASSET_DIR);
    g_scene.buildLayouts(world, SCENE_COLS, SCENE_ROWS);
    g_sceneLoaded = true;
}

void unloadSceneAssets() {
    g_scene.unloadAll();
}

// -----------------------------------------------------------------------
// Palette
// -----------------------------------------------------------------------
static const Color C_BG          = {  15,  15,  20, 255 };  // near-black
static const Color C_PANEL       = {  25,  25,  35, 255 };  // dark panel
static const Color C_BORDER      = {  60,  60,  90, 255 };  // muted purple border
static const Color C_ACCENT      = { 180, 140,  60, 255 };  // gold
static const Color C_TEXT        = { 210, 210, 210, 255 };  // off-white
static const Color C_DIM         = { 100, 100, 120, 255 };  // dim gray
static const Color C_INPUT_BG    = {  20,  30,  20, 255 };  // dark green tint
static const Color C_INPUT_TEXT  = {  80, 220,  80, 255 };  // bright green
static const Color C_HP          = { 200,  60,  60, 255 };  // red
static const Color C_HP_BG       = {  60,  20,  20, 255 };
static const Color C_EN          = {  60, 160, 220, 255 };  // blue
static const Color C_EN_BG       = {  20,  40,  60, 255 };
static const Color C_COMBAT      = { 240, 120,  20, 255 };  // orange
static const Color C_NPC         = { 140, 200, 140, 255 };  // soft green
static const Color C_ITEM        = { 200, 180, 100, 255 };  // warm yellow
static const Color C_EXIT        = { 120, 160, 200, 255 };  // soft blue

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static void drawPanel(Rectangle r, Color bg, Color border) {
    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, 1.5f, border);
}

// draw a filled bar (health / energy)
static void drawBar(int x, int y, int w, int h,
                    float fraction, Color bg, Color fill, const std::string& label) {
    DrawRectangle(x, y, w, h, bg);
    DrawRectangle(x, y, (int)(w * fraction), h, fill);
    DrawRectangleLinesEx({(float)x,(float)y,(float)w,(float)h}, 1.0f, C_BORDER);
    DrawText(label.c_str(), x + 4, y + (h - FS_SMALL())/2, FS_SMALL(), WHITE);
}

// simple word-wrap: draw text inside a rect, return new Y after last line
static int drawWrapped(const std::string& text, int x, int y, int maxW,
                       int fontSize, Color col, int maxLines = 8) {
    int charsPerLine = maxW / (fontSize / 2 + 1);
    if (charsPerLine < 1) charsPerLine = 1;
    size_t pos = 0;
    int lines = 0;
    while (pos < text.size() && lines < maxLines) {
        std::string line = text.substr(pos, charsPerLine);
        if (pos + charsPerLine < text.size()) {
            size_t sp = line.rfind(' ');
            if (sp != std::string::npos) line = line.substr(0, sp);
        }
        DrawText(line.c_str(), x, y, fontSize, col);
        pos += line.size() + 1;
        y   += fontSize + 4;
        lines++;
    }
    return y;
}

// -----------------------------------------------------------------------
// drawHUD
// -----------------------------------------------------------------------
static void drawHUD(const Player& player) {
    Rectangle hudPanel = HUD_PANEL();
    drawPanel(hudPanel, C_PANEL, C_BORDER);

    int x = (int)hudPanel.x + (int)(8 * SX());
    int y = (int)hudPanel.y + (int)(10 * SY());
    int barW = std::max(60, (int)((hudPanel.width - 20 * SX()) / 2.0f));
    int barH = (int)(18 * SY());

    // Health bar
    float hpFrac = (player.MaxHealth() > 0)
        ? (float)(player.getHealth() / player.MaxHealth()) : 0.f;
    if (hpFrac < 0.f) hpFrac = 0.f;
    if (hpFrac > 1.f) hpFrac = 1.f;
    std::string hpLabel = "HP " + std::to_string((int)player.getHealth())
                        + "/" + std::to_string((int)player.MaxHealth());
    drawBar(x, y + barH + (int)(SY()), barW+60, barH, hpFrac, C_HP_BG, C_HP, hpLabel);

    // Energy bar
    float enFrac = (player.MaxEnergy() > 0)
        ? (float)(player.getEnergy() / player.MaxEnergy()) : 0.f;
    if (enFrac < 0.f) enFrac = 0.f;
    if (enFrac > 1.f) enFrac = 1.f;
    std::string enLabel = "EN " + std::to_string((int)player.getEnergy())
                        + "/" + std::to_string((int)player.MaxEnergy());
    drawBar(x, y + barH + (int)(20 * SY()), barW+60, barH, enFrac, C_EN_BG, C_EN, enLabel);

    // Level + name
    std::string lvl = "LVL " + std::to_string(player.getLevel())
                    + "   " + player.getName();
    DrawText(lvl.c_str(), x, y, FS_SMALL(), C_ACCENT);

    // Combat flash
    if (player.getInCombat()) {
        int boxW = (int)(hudPanel.width - 16 * SX());
        int cx = (int)hudPanel.x + (int)(8 * SX());
        DrawRectangle(cx, y + barH + (int)(28 * SY()), boxW, (int)(18 * SY()), C_COMBAT);
        DrawText("[ IN COMBAT ]", cx + (int)(6 * SX()), y + barH + (int)(30 * SY()), FS_SMALL(), BLACK);
    }

    // Stats grid: 2 rows x 2 columns
    int statsY = y + barH + (int)(45 * SY());
    int statsW = (int)((hudPanel.width - 16 * SX()) / 2.0f);
    int statsH = (int)(18 * SY());
    int statsGap = (int)(6 * SY());
    const std::array<std::pair<std::string, int>, 4> statItems = {{
        {"STR", (int)player.getStats().strength},
        {"DEX", (int)player.getStats().dexterity},
        {"INT", (int)player.getStats().intelligence},
        {"DEF", (int)player.getStats().defence}
    }};

    for (size_t i = 0; i < statItems.size(); ++i) {
        int row = (int)(i / 2);
        int col = (int)(i % 2);
        int sx = x + col * statsW;
        int sy = statsY + row * (statsH + statsGap);
        std::string statText = statItems[i].first + " " + std::to_string(statItems[i].second);
        DrawText(statText.c_str(), sx, sy, FS_SMALL(), C_DIM);
    }
}

// -----------------------------------------------------------------------
// drawRoom — relX/relY are 0..1 fractions of the scene viewport locating
// Thomas (0.5,0.5 = dead-center; 0,0 = top-left corner).
// -----------------------------------------------------------------------
static void drawRoom(const Room* room, SpriteAnimator& thomas, float relX, float relY) {
    if (!room) return;

    Rectangle roomPanel = ROOM_PANEL();
    drawPanel(roomPanel, C_PANEL, C_BORDER);

    int x  = (int)roomPanel.x + (int)PAD();
    int y  = (int)roomPanel.y + (int)PAD();
    int mW = (int)roomPanel.width - (int)PAD() * 2;

    // Room name
    DrawText(room->getName().c_str(), x, y, FS_TITLE(), C_ACCENT);
    y += FS_TITLE() + (int)(6 * SY());
    DrawLine(x, y, x + mW, y, C_BORDER);
    y += (int)(10 * SY());

    // Scene viewport: tile background, animated props, NPCs, Thomas.
    // Tile art itself stays at its native size (see note near SCENE_H above) —
    // only the surrounding box position/height scale with the window.
    int sceneH = (int)(SCENE_H * SY());
    Rectangle sceneRect = { (float)x, (float)y, (float)mW, (float)sceneH };
    DrawRectangleRec(sceneRect, BLACK);
    g_scene.drawFloor(room->getName(), x, y, TILE_SCALE);
    g_scene.drawDecor(room->getName(), x, y, TILE_SCALE);
    g_scene.drawDecorFeatures(room->getName(), x, y, TILE_SCALE);  // doors, banners, structures
    g_scene.drawProps(room->getName(), x, y, mW, sceneH, TILE_SCALE);
    g_scene.drawNpcs(room->getNpcEntities(), x, y, mW, sceneH, TILE_SCALE);
    thomas.draw(x + (int)(relX * mW), y + (int)(relY * sceneH), 2.0f);
    DrawRectangleLinesEx(sceneRect, 1.5f, C_BORDER);
    y += sceneH + (int)(10 * SY());

    // Description
    y = drawWrapped(room->getDescription(), x, y, mW, FS(), C_TEXT, 3);
    y += (int)(8 * SY());

    // Two-column layout: NPCs + Items left, Exits right
    int colW  = mW / 2 - (int)(8 * SX());
    int leftX = x;
    int rightX = x + colW + (int)(16 * SX());
    int leftY  = y;
    int rightY = y;

    // NPCs
    if (!room->getNpcEntities().empty()) {
        DrawText("Characters", leftX, leftY, FS_SMALL(), C_DIM);
        leftY += LINE_H() - (int)(2 * SY());
        for (const auto& npc : room->getNpcEntities()) {
            std::string line = "  * " + npc->getName();
            DrawText(line.c_str(), leftX, leftY, FS(), C_NPC);
            leftY += LINE_H();
        }
        leftY += (int)(6 * SY());
    }

    // Items
    if (!room->getItems().empty()) {
        DrawText("Items", leftX, leftY, FS_SMALL(), C_DIM);
        leftY += LINE_H() - (int)(2 * SY());
        for (const auto& item : room->getItems()) {
            std::string line = "  + " + item->getName();
            DrawText(line.c_str(), leftX, leftY, FS(), C_ITEM);
            leftY += LINE_H();
        }
    }

    // Exits
    DrawText("Exits", rightX, rightY, FS_SMALL(), C_DIM);
    rightY += LINE_H() - (int)(2 * SY());
    for (const auto& e : room->getExits()) {
        std::string line = "  " + e.first + " -> " + e.second->getName();
        DrawText(line.c_str(), rightX, rightY, FS(), C_EXIT);
        rightY += LINE_H();
    }

    // Divider between columns
    int divX = x + colW + (int)(8 * SX());
    DrawLine(divX, y, divX, (int)roomPanel.y + (int)roomPanel.height - (int)PAD(), C_BORDER);
}

// -----------------------------------------------------------------------
// drawDialogue — shown instead of the room view whenever dialogue.active
// -----------------------------------------------------------------------
static void drawDialogue(const DialogueState& dlg) {
    if (!dlg.active) return;
    Rectangle panel = ROOM_PANEL(); // reuse the room panel's footprint
    drawPanel(panel, C_PANEL, C_ACCENT);

    int x = (int)panel.x + (int)PAD();
    int y = (int)panel.y + (int)PAD();
    int mW = (int)panel.width - (int)PAD() * 2;

    DrawText(dlg.speaker.c_str(), x, y, FS_TITLE(), C_ACCENT);
    y += FS_TITLE() + (int)(10 * SY());

    for (const auto& line : dlg.lines) {
        y = drawWrapped(line, x, y, mW, FS(), C_TEXT, 4);
        y += (int)(6 * SY());
    }

    y += (int)(10 * SY());
    for (const auto& opt : dlg.options) {
        DrawText(opt.c_str(), x, y, FS(), C_EXIT);
        y += LINE_H();
    }

    if (dlg.options.empty()) {
        DrawText("(press Enter to continue)", x, y, FS_SMALL(), C_DIM);
    }
}

// -----------------------------------------------------------------------
// drawInputBar
// -----------------------------------------------------------------------
static void drawInputBar(const std::string& inputBuffer) {
    Rectangle inputPanel = INPUT_PANEL();
    drawPanel(inputPanel, C_INPUT_BG, C_BORDER);

    int x = (int)inputPanel.x + (int)PAD();
    int y = (int)inputPanel.y + (int)(10 * SY());

    DrawText("Command:", x, y, FS_SMALL(), C_DIM);

    // blinking cursor — visible for 30 frames, hidden for 30
    bool cursorOn = (GetTime() * 2.0 - (int)(GetTime() * 2.0)) < 0.5;
    std::string prompt = "> " + inputBuffer + (cursorOn ? "|" : " ");
    DrawText(prompt.c_str(), x + (int)(80 * SX()), y, FS(), C_INPUT_TEXT);

    // hint
    DrawText("Type a command and press Enter.  'help' for a list.",
             x, y + LINE_H(), FS_SMALL(), C_DIM);
}

// -----------------------------------------------------------------------
// Mini-map — lays out rooms on a grid by walking the exit graph. Rebuilt
// automatically whenever the room count changes, since the world grows at
// runtime (createMaze()/createMazePhaseTwo() add rooms as the game progresses).
// -----------------------------------------------------------------------
static std::unordered_map<const Room*, std::pair<int,int>> g_mapCoords;
static size_t g_mapRoomCount = 0;

static std::pair<int,int> dirOffset(const std::string& dir) {
    if (dir == "north" )   return {0, -1};
    if (dir == "south") return {0,  1};
    if (dir == "east")                   return {1,  0};
    if (dir == "west")                   return {-1, 0};
    return {0, 0}; // unrecognized direction — stacks on the same cell, still visible
}

static void buildMiniMap(const World& world) {
    g_mapCoords.clear();
    const auto& rooms = world.getRooms();
    if (rooms.empty()) return;

    std::queue<const Room*> q;
    q.push(rooms[0]);
    g_mapCoords[rooms[0]] = {0, 0};

    while (!q.empty()) {
        const Room* r = q.front(); q.pop();
        std::pair<int,int> pos = g_mapCoords[r];
        for (const auto& e : r->getExits()) {
            const Room* next = e.second;
            if (g_mapCoords.count(next)) continue;
            std::pair<int,int> off = dirOffset(e.first);
            g_mapCoords[next] = { pos.first + off.first, pos.second + off.second };
            q.push(next);
        }
    }
    g_mapRoomCount = rooms.size();
}

static void drawMiniMap(const World& world, const Room* current, Rectangle area) {
    if (world.getRooms().size() != g_mapRoomCount) {
        buildMiniMap(world);
    }
    if (g_mapCoords.empty()) return;

    int minX = INT32_MAX, maxX = INT32_MIN, minY = INT32_MAX, maxY = INT32_MIN;
    for (const auto& kv : g_mapCoords) {
        minX = std::min(minX, kv.second.first);
        maxX = std::max(maxX, kv.second.first);
        minY = std::min(minY, kv.second.second);
        maxY = std::max(maxY, kv.second.second);
    }
    int spanX = std::max(1, maxX - minX);
    int spanY = std::max(1, maxY - minY);

    float cell = std::min(area.width / (spanX + 1), area.height / (spanY + 1));
    float gridW = cell * (spanX + 1);
    float gridH = cell * (spanY + 1);
    float originX = area.x + (area.width  - gridW) / 2.0f;
    float originY = area.y + (area.height - gridH) / 2.0f;

    auto cellCenter = [&](int gx, int gy) -> Vector2 {
        return { originX + (gx - minX) * cell + cell / 2.0f,
                 originY + (gy - minY) * cell + cell / 2.0f };
    };

    // Connections first, so room markers draw on top of the lines
    for (const Room* r : world.getRooms()) {
        auto itA = g_mapCoords.find(r);
        if (itA == g_mapCoords.end()) continue;
        Vector2 a = cellCenter(itA->second.first, itA->second.second);
        for (const auto& e : r->getExits()) {
            auto itB = g_mapCoords.find(e.second);
            if (itB == g_mapCoords.end()) continue;
            Vector2 b = cellCenter(itB->second.first, itB->second.second);
            DrawLineEx(a, b, 1.5f, C_BORDER);
        }
    }

    // Room markers — bigger/highlighted for the player's current room
    float markerR = std::max(2.0f, cell * 0.22f);
    for (const auto& kv : g_mapCoords) {
        Vector2 c = cellCenter(kv.second.first, kv.second.second);
        bool isCurrent = (kv.first == current);
        DrawCircleV(c, isCurrent ? markerR * 1.6f : markerR, isCurrent ? C_ACCENT : C_DIM);
        if (isCurrent) DrawCircleLines((int)c.x, (int)c.y, (int)(markerR * 1.6f) + 2, WHITE);
    }
}

static void drawPortrait(const World& world, const Player& player) {
    Rectangle panel = MINIMAP_PANEL();
    DrawRectangleRec(panel, C_PANEL);
    DrawRectangleLinesEx(panel, 1.5f, C_BORDER);

    DrawText(player.getName().c_str(), (int)panel.x + (int)(8 * SX()), (int)panel.y + (int)(8 * SY()), (int)(14 * SY()), C_ACCENT);

    Rectangle mapArea = { panel.x + 8 * SX(), panel.y + 28 * SY(), panel.width - 16 * SX(), panel.height - 56 * SY() };
    drawMiniMap(world, player.getLocation(), mapArea);

    std::string roomLabel = player.getLocation() ? player.getLocation()->getName() : "";
    DrawText(roomLabel.c_str(), (int)panel.x + (int)(8 * SX()), (int)(panel.y + panel.height - 20 * SY()), (int)(12 * SY()), C_DIM);
}

// -----------------------------------------------------------------------
// drawGame — entry point called every frame from main.cpp
// -----------------------------------------------------------------------
void drawGame(const World& world, const Player& player,
              const std::string& inputBuffer, SpriteAnimator& animator,
              const DialogueState& dialogue, float playerRelX, float playerRelY) {
    ensureSceneLoaded(world);
    g_scene.update(GetFrameTime());

    BeginDrawing();
    ClearBackground(C_BG);
    if (dialogue.active) {
        drawDialogue(dialogue);
    } else {
        drawRoom(player.getLocation(), animator, playerRelX, playerRelY);
    }
    drawHUD(player);
    drawPortrait(world, player);
    drawInputBar(inputBuffer);
    EndDrawing();
}