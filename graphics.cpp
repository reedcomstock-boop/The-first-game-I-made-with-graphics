#include "graphics.h"
#include "raylib.h"
#include "roomscene.h"
#include "updater.h"
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
static const int32_t BASE_ScreenWidth = 800;
static const int32_t BASE_ScreenHigh = 600;
static const int32_t BASE_PAD = 16;

static inline float ScreenWidth() { return (float)GetScreenWidth(); } 
static inline float ScreenHeight() { return (float)GetScreenHeight(); }
static inline float SX() { return ScreenWidth() / (float)BASE_ScreenWidth; }  // width scale factor
static inline float SY() { return ScreenHeight() / (float)BASE_ScreenHigh; }  // height scale factor

static inline float PAD()      { return BASE_PAD   * SX(); }
static inline int32_t   LINE_H()   { return (int32_t)(22   * SY()); }
static inline int32_t   FS()       { return (int32_t)(16   * SY()); }   // base font size
static inline int32_t   FS_TITLE() { return (int32_t)(22   * SY()); }
static inline int32_t   FS_SMALL() { return (int32_t)(13   * SY()); }

// Panel rects — percentages of current screen size
static inline Rectangle MINIMAP_PANEL() {
    return { 624 * SX(), 16 * SY(), 160 * SX(), 200 * SY() };
}
static inline Rectangle ROOM_PANEL() {
    Rectangle map = MINIMAP_PANEL();
    float gap = 8 * SX();
    return { PAD(), PAD(), map.x - gap - PAD(), (BASE_ScreenHigh - 100) * SY() };
}
static inline Rectangle HUD_PANEL() {
    Rectangle map = MINIMAP_PANEL();
    return { map.x, map.y + map.height + 8 * SY(), map.width, 200 * SY() };
}
static inline Rectangle INPUT_PANEL() {
    return { PAD(), (BASE_ScreenHigh - 75) * SY(), ScreenWidth() - PAD()*2.0f, 50 * SY() };
}

 
static inline Rectangle Game_Clock_PANEL() {
    Rectangle hud   = HUD_PANEL();
    Rectangle input = INPUT_PANEL();
    float gap = 8 * SY();
    float top    = hud.y + hud.height + gap;
    float bottom = input.y - gap;
    return { hud.x, top, hud.width, bottom - top };
}

// -----------------------------------------------------------------------
// Scene view (tile background + NPCs + animated props)
// Expects an "assets" folder next to the binary: assets/tiles, assets/npc, assets/props
// -----------------------------------------------------------------------
static const char* ASSET_DIR   = "assets";
//static const float TILE_SCALE  = 2.0f;
static const int32_t   TILE_DRAW   = 32;       // 16px source tile * TILE_SCALE
static const int32_t   SCENE_HEIGHT     = 250;      // height of the scene viewport inside ROOM_PANEL (base pixels, scaled by SY() at draw time)
static const int32_t   SCENE_COLS  = (int32_t)(BASE_ScreenWidth - BASE_PAD*2 - BASE_PAD*2) / TILE_DRAW;
static const int32_t   SCENE_ROWS  = SCENE_HEIGHT / TILE_DRAW;

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
static void drawBar(int32_t x, int32_t y, int32_t w, int32_t h,
                    float fraction, Color bg, Color fill, const std::string& label) {
    DrawRectangle(x, y, w, h, bg);
    DrawRectangle(x, y, (int32_t)(w * fraction), h, fill);
    DrawRectangleLinesEx({(float)x,(float)y,(float)w,(float)h}, 1.0f, C_BORDER);
    DrawText(label.c_str(), x + 4, y + (h - FS_SMALL())/2, FS_SMALL(), WHITE);
}

// simple word-wrap: draw text inside a rect, return new Y after last line
static int32_t drawWrapped(const std::string& text, int32_t x, int32_t y, int32_t maxW,
                       int32_t fontSize, Color col, int32_t maxLines = 8) {
    int32_t charsPerLine = maxW / (fontSize / 2 + 1);
    if (charsPerLine < 1) charsPerLine = 1;
    size_t pos = 0;
    int32_t lines = 0;
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
//-----------------------------------------------------------------------
// drawGameClock
// -----------------------------------------------------------------------
/*static void drawGameClock() {
    int32_t updateCount = Updater::getUpdateCount();
    std::string text = "Game Clock: " + std::to_string(updateCount);
    DrawText(text.c_str(), 10, 10, FS_SMALL, WHITE);
*/// -----------------------------------------------------------------------
// drawGameTime
// -----------------------------------------------------------------------
static void drawGameClock() {
    Rectangle clockPanel = Game_Clock_PANEL();
    drawPanel(clockPanel, C_PANEL, C_BORDER);

    int32_t sceneH = (int32_t)(SCENE_HEIGHT * SY());
    float sceneScale = (float)sceneH / (float)(SCENE_ROWS * 16);
    
    int32_t gameClock = Updater::getGameClock();
    int32_t updateCount = Updater::getUpdateCount();
    if (gameClock >= 24) {
        int32_t days = gameClock / 24;
        gameClock = 0;
        std::string text = "Game Clock: " + std::to_string(days) + " days ";
        std::string text2 = "           " + std::to_string(gameClock) + ":" + std::to_string(updateCount * 19);
        int32_t textX = (int32_t)clockPanel.x + (int32_t)(8 * SX());
        int32_t textY = (int32_t)(clockPanel.y + (clockPanel.height -FS()) / sceneScale);
        DrawText(text.c_str(), textX, textY, FS(), WHITE);
        textY += FS_SMALL() + 4;
        DrawText(text2.c_str(), textX, textY, FS(), WHITE);
    }
    else {
        std::string text = "Game Clock: " + std::to_string(gameClock) + ":" + std::to_string(updateCount * 19);
        int32_t textX = (int32_t)clockPanel.x + (int32_t)(8 * SX());
        int32_t textY = (int32_t)(clockPanel.y + (clockPanel.height - FS()) / sceneScale);
        DrawText(text.c_str(), textX, textY, FS(), WHITE);
    }
    
    
    std::string text2 = std::to_string(gameClock) + ":" + std::to_string(updateCount * 19);

    //int32_t textX2 = (int32_t)clockPanel.x + (int32_t)(8 * SX());
    //int32_t textY2 = (int32_t)(clockPanel.y  + 10 + (clockPanel.height - FS_SMALL()) / 2.0f);
    //DrawText(text2.c_str(), textX2, textY2, FS_(), WHITE);
}
// -----------------------------------------------------------------------
// drawHUD
// -----------------------------------------------------------------------
static void drawHUD(const Player& player) {
    Rectangle hudPanel = HUD_PANEL();
    drawPanel(hudPanel, C_PANEL, C_BORDER);

    int32_t x = (int32_t)hudPanel.x + (int32_t)(8 * SX());
    int32_t y = (int32_t)hudPanel.y + (int32_t)(10 * SY());
    int32_t barW = std::max(60, (int32_t)((hudPanel.width - 20 * SX()) / 2.0f));
    int32_t barH = (int32_t)(18 * SY());

    // Health bar
    float hpFrac = (player.MaxHealth() > 0)
        ? (float)(player.getHealth() / player.MaxHealth()) : 0.f;
    if (hpFrac < 0.f) hpFrac = 0.f;
    if (hpFrac > 1.f) hpFrac = 1.f;
    std::string hpLabel = "HP " + std::to_string((int32_t)player.getHealth())
                        + "/" + std::to_string((int32_t)player.MaxHealth());
    drawBar(x, y + barH + (int32_t)(SY()), barW+60, barH, hpFrac, C_HP_BG, C_HP, hpLabel);

    // Energy bar
    float enFrac = (player.MaxEnergy() > 0)
        ? (float)(player.getEnergy() / player.MaxEnergy()) : 0.f;
    if (enFrac < 0.f) enFrac = 0.f;
    if (enFrac > 1.f) enFrac = 1.f;
    std::string enLabel = "EN " + std::to_string((int32_t)player.getEnergy())
                        + "/" + std::to_string((int32_t)player.MaxEnergy());
    drawBar(x, y + barH + (int32_t)(20 * SY()), barW+60, barH, enFrac, C_EN_BG, C_EN, enLabel);

    // Level + name
    std::string lvl = "LVL " + std::to_string(player.getLevel())
                    + "   " + player.getName();
    DrawText(lvl.c_str(), x, y, FS_SMALL(), C_ACCENT);

    // Combat flash
    if (player.getInCombat()) {
        int32_t boxW = (int32_t)(hudPanel.width - 16 * SX());
        int32_t cx = (int32_t)hudPanel.x + (int32_t)(8 * SX());
        DrawRectangle(cx, y + barH + (int32_t)(28 * SY()), boxW, (int32_t)(18 * SY()), C_COMBAT);
        DrawText("[ IN COMBAT ]", cx + (int32_t)(6 * SX()), y + barH + (int32_t)(30 * SY()), FS_SMALL(), BLACK);
    }

    // Stats grid: 2 rows x 2 columns
    int32_t statsY = y + barH + (int32_t)(56 * SY());
    int32_t statsW = (int32_t)((hudPanel.width - 16 * SX()) / 2.0f);
    int32_t statsH = (int32_t)(25 * SY());
    int32_t statsGap = (int32_t)(6 * SY());
    const std::array<std::pair<std::string, int32_t>, 4> statItems = {{
        {"STR", (int32_t)player.getStats().strength},
        {"DEX", (int32_t)player.getStats().dexterity},
        {"INT", (int32_t)player.getStats().intelligence},
        {"DEF", (int32_t)player.getStats().defence}
    }};

    for (size_t i = 0; i < statItems.size(); ++i) {
        int32_t row = (int32_t)(i / 2);
        int32_t col = (int32_t)(i % 2);
        int32_t sx = x + col * statsW;
        int32_t sy = statsY + row * (statsH + statsGap);
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

    int32_t x  = (int32_t)roomPanel.x + (int32_t)PAD();
    int32_t y  = (int32_t)roomPanel.y + (int32_t)PAD();
    int32_t mW = (int32_t)roomPanel.width - (int32_t)PAD() * 2;

    // Room name
    DrawText(room->getName().c_str(), x, y, FS_TITLE(), C_ACCENT);
    y += FS_TITLE() + (int32_t)(6 * SY());
    DrawLine(x, y, x + mW, y, C_BORDER);
    y += (int32_t)(10 * SY());

    // Scene viewport: tile background, animated props, NPCs, Thomas.
    // Tile art itself stays at its native size (see note near SCENE_HEIGHT above) —
    // only the surrounding box position/height scale with the window.
    int32_t sceneH = (int32_t)(SCENE_HEIGHT * SY());
    Rectangle sceneRect = { (float)x, (float)y, (float)mW, (float)sceneH };

    // Always fill the box vertically — never leave black bars top/bottom.
    // If the resulting width is wider than the panel, we clip it with
    // scissor mode below rather than shrinking the scale to fit width.
    float sceneScale = (float)sceneH / (float)(SCENE_ROWS * 16);

    DrawRectangleRec(sceneRect, BLACK);
    BeginScissorMode((int32_t)sceneRect.x, (int32_t)sceneRect.y, (int32_t)sceneRect.width, (int32_t)sceneRect.height);

    g_scene.drawFloor(room->getName(), x, y, sceneScale);
    g_scene.drawDecor(room->getName(), x, y, sceneScale);
    g_scene.drawDecorFeatures(room->getName(), x, y, sceneScale);  // doors, banners, structures
    g_scene.drawProps(room->getName(), x, y, mW, sceneH, sceneScale);
    g_scene.drawNpcs(room->getNpcEntities(), x, y, mW, sceneH, sceneScale);
    thomas.draw(x + (int32_t)(relX * mW), y + (int32_t)(relY * sceneH), sceneScale);
    EndScissorMode();
    DrawRectangleLinesEx(sceneRect, 1.5f, C_BORDER);
    y += sceneH + (int32_t)(10 * SY());

    // Description
    y = drawWrapped(room->getDescription(), x, y, mW, FS(), C_TEXT, 3);
    y += (int32_t)(8 * SY());

    // Two-column layout: NPCs + Items left, Exits right
    int32_t colW  = mW / 2 - (int32_t)(8 * SX());
    int32_t leftX = x;
    int32_t rightX = x + colW + (int32_t)(16 * SX());
    int32_t leftY  = y;
    int32_t rightY = y;

    // NPCs
    if (!room->getNpcEntities().empty()) {
        DrawText("Characters", leftX, leftY, FS_SMALL(), C_DIM);
        leftY += LINE_H() - (int32_t)(2 * SY());
        for (const auto& npc : room->getNpcEntities()) {
            std::string line = "  * " + npc->getName();
            DrawText(line.c_str(), leftX, leftY, FS(), C_NPC);
            leftY += LINE_H();
        }
        leftY += (int32_t)(6 * SY());
    }

    // Items
    if (!room->getItems().empty()) {
        DrawText("Items", leftX, leftY, FS_SMALL(), C_DIM);
        leftY += LINE_H() - (int32_t)(2 * SY());
        for (const auto& item : room->getItems()) {
            std::string line = "  + " + item->getName();
            DrawText(line.c_str(), leftX, leftY, FS(), C_ITEM);
            leftY += LINE_H();
        }
    }

    // Exits
    DrawText("Exits", rightX, rightY, FS_SMALL(), C_DIM);
    rightY += LINE_H() - (int32_t)(2 * SY());
    for (const auto& e : room->getExits()) {
        std::string line = "  " + e.first + " -> " + e.second->getName();
        DrawText(line.c_str(), rightX, rightY, FS(), C_EXIT);
        rightY += LINE_H();
    }

    // Divider between columns
    int32_t divX = x + colW + (int32_t)(8 * SX());
    DrawLine(divX, y, divX, (int32_t)roomPanel.y + (int32_t)roomPanel.height - (int32_t)PAD(), C_BORDER);
}

// -----------------------------------------------------------------------
// drawDialogue — shown instead of the room view whenever dialogue.active
// -----------------------------------------------------------------------
static void drawDialogue(const DialogueState& dlg) {
    if (!dlg.active) return;
    Rectangle panel = ROOM_PANEL();
    drawPanel(panel, C_PANEL, C_ACCENT);

    int32_t x = (int32_t)panel.x + (int32_t)PAD();
    int32_t y = (int32_t)panel.y + (int32_t)PAD();
    int32_t mW = (int32_t)panel.width - (int32_t)PAD() * 2;

    bool viewingHistory = dlg.scrollOffset > 0;

    DrawText(dlg.speaker.c_str(), x, y, FS_TITLE(), C_ACCENT);
    y += FS_TITLE() + (int32_t)(10 * SY());

    if (viewingHistory) {
        // Show a scrolled-back slice of the transcript instead of the live page.
        DrawText("(scrolled back — press Down to return, Up for older)",
                 x, y, FS_SMALL(), C_DIM);
        y += LINE_H();

        int32_t endIdx   = std::max(0, (int32_t)dlg.history.size() - dlg.scrollOffset);
        int32_t startIdx = std::max(0, endIdx - 5);   // show up to 5 transcript entries at a time
        for (int32_t i = startIdx; i < endIdx; i++) {
            y = drawWrapped(dlg.history[i], x, y, mW, FS(), C_TEXT, 3);
            y += (int32_t)(4 * SY());
        }
        return;   // options never show while scrolled back — they belong to the live page
    }

    for (const auto& line : dlg.lines) {
        y = drawWrapped(line, x, y, mW, FS(), C_TEXT, 4);
        y += (int32_t)(6 * SY());
    }

    y += (int32_t)(10 * SY());
    for (const auto& opt : dlg.options) {
        DrawText(opt.c_str(), x, y, FS(), C_EXIT);
        y += LINE_H();
    }

    if (dlg.options.empty()) {
        DrawText("(press Enter to continue)", x, y, FS_SMALL(), C_DIM);
    } else {
        DrawText("(Up arrow to scroll back through this conversation)",
                 x, y, FS_SMALL(), C_DIM);
    }
}

// -----------------------------------------------------------------------
// drawInputBar
// -----------------------------------------------------------------------
static void drawInputBar(const std::string& inputBuffer) {
    Rectangle inputPanel = INPUT_PANEL();
    drawPanel(inputPanel, C_INPUT_BG, C_BORDER);

    int32_t x = (int32_t)inputPanel.x + (int32_t)PAD();
    int32_t y = (int32_t)inputPanel.y + (int32_t)(10 * SY());

    DrawText("Command:", x, y, FS_SMALL(), C_DIM);

    // blinking cursor — visible for 30 frames, hidden for 30
    bool cursorOn = (GetTime() * 2.0 - (int32_t)(GetTime() * 2.0)) < 0.5;
    std::string prompt = "> " + inputBuffer + (cursorOn ? "|" : " ");
    DrawText(prompt.c_str(), x + (int32_t)(80 * SX()), y, FS(), C_INPUT_TEXT);

    // hint
    DrawText("Type a command and press Enter.  'help' for a list.",
             x, y + LINE_H(), FS_SMALL(), C_DIM);
}

// -----------------------------------------------------------------------
// Mini-map — lays out rooms on a grid by walking the exit graph. Rebuilt
// automatically whenever the room count changes, since the world grows at
// runtime (createMaze()/createMazePhaseTwo() add rooms as the game progresses).
// -----------------------------------------------------------------------
static std::unordered_map<const Room*, std::pair<int32_t,int32_t>> g_mapCoords;
static size_t g_mapRoomCount = 0;

static std::pair<int32_t,int32_t> dirOffset(const std::string& dir) {
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
        std::pair<int32_t,int32_t> pos = g_mapCoords[r];
        for (const auto& e : r->getExits()) {
            const Room* next = e.second;
            if (g_mapCoords.count(next)) continue;
            std::pair<int32_t,int32_t> off = dirOffset(e.first);
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

    int32_t minX = INT32_MAX, maxX = INT32_MIN, minY = INT32_MAX, maxY = INT32_MIN;
    for (const auto& kv : g_mapCoords) {
        minX = std::min(minX, kv.second.first);
        maxX = std::max(maxX, kv.second.first);
        minY = std::min(minY, kv.second.second);
        maxY = std::max(maxY, kv.second.second);
    }
    int32_t spanX = std::max(1, maxX - minX);
    int32_t spanY = std::max(1, maxY - minY);

    float cell = std::min(area.width / (spanX + 1), area.height / (spanY + 1));
    float gridW = cell * (spanX + 1);
    float gridH = cell * (spanY + 1);
    float originX = area.x + (area.width  - gridW) / 2.0f;
    float originY = area.y + (area.height - gridH) / 2.0f;

    auto cellCenter = [&](int32_t gx, int32_t gy) -> Vector2 {
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
        if (isCurrent) DrawCircleLines((int32_t)c.x, (int32_t)c.y, (int32_t)(markerR * 1.6f) + 2, WHITE);
    }
}

static void drawPortrait(const World& world, const Player& player) {
    Rectangle panel = MINIMAP_PANEL();
    DrawRectangleRec(panel, C_PANEL);
    DrawRectangleLinesEx(panel, 1.5f, C_BORDER);

    DrawText(player.getName().c_str(), (int32_t)panel.x + (int32_t)(8 * SX()), (int32_t)panel.y + (int32_t)(8 * SY()), (int32_t)(14 * SY()), C_ACCENT);

    Rectangle mapArea = { panel.x + 8 * SX(), panel.y + 28 * SY(), panel.width - 16 * SX(), panel.height - 56 * SY() };
    drawMiniMap(world, player.getLocation(), mapArea);

    std::string roomLabel = player.getLocation() ? player.getLocation()->getName() : "";
    DrawText(roomLabel.c_str(), (int32_t)panel.x + (int32_t)(8 * SX()), (int32_t)(panel.y + panel.height - 20 * SY()), (int32_t)(12 * SY()), C_DIM);
}

// -----------------------------------------------------------------------
// drawGame — entry point called every frame from main.cpp
// -----------------------------------------------------------------------
void drawGame(const World& world, const Player& player,
              const std::string& inputBuffer, SpriteAnimator& animator,
              const DialogueState& dialogue, float playerRelX, float playerRelY) {
    ensureSceneLoaded(world);
    g_scene.update(GetFrameTime());
    drawGameClock();    //drawGameClock();
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