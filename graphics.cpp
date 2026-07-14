#include "graphics.h"
#include "raylib.h"
#include "roomscene.h"
#include <string>

// -----------------------------------------------------------------------
// Layout
// -----------------------------------------------------------------------
static const int SW        = 800;
static const int SH        = 600;
static const int PAD       = 16;
static const int LINE_H    = 22;
static const int FS        = 16;   // base font size
static const int FS_TITLE  = 22;
static const int FS_SMALL  = 13;

// Panel rects
static const Rectangle ROOM_PANEL  = { PAD,        PAD,        SW - PAD*2,      SH - 200 };
static const Rectangle HUD_PANEL   = { PAD,        SH - 185,   SW - PAD*2,      100      };
static const Rectangle INPUT_PANEL = { PAD,        SH - 75,    SW - PAD*2,      50       };

// -----------------------------------------------------------------------
// Scene view (tile background + NPCs + animated props)
// Expects an "assets" folder next to the binary: assets/tiles, assets/npc, assets/props
// -----------------------------------------------------------------------
static const char* ASSET_DIR   = "assets";
static const float TILE_SCALE  = 2.0f;
static const int   TILE_DRAW   = 32;       // 16px source tile * TILE_SCALE
static const int   SCENE_H     = 150;      // height of the scene viewport inside ROOM_PANEL
static const int   SCENE_COLS  = (int)(SW - PAD*2 - PAD*2) / TILE_DRAW;
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
    DrawText(label.c_str(), x + 4, y + (h - FS_SMALL)/2, FS_SMALL, WHITE);
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
    drawPanel(HUD_PANEL, C_PANEL, C_BORDER);

    int x = (int)HUD_PANEL.x + PAD;
    int y = (int)HUD_PANEL.y + 10;
    int barW = 180;
    int barH = 18;

    // Health bar
    float hpFrac = (player.MaxHealth() > 0)
        ? (float)(player.getHealth() / player.MaxHealth()) : 0.f;
    if (hpFrac < 0.f) hpFrac = 0.f;
    if (hpFrac > 1.f) hpFrac = 1.f;
    std::string hpLabel = "HP " + std::to_string((int)player.getHealth())
                        + "/" + std::to_string((int)player.MaxHealth());
    drawBar(x, y, barW, barH, hpFrac, C_HP_BG, C_HP, hpLabel);

    // Energy bar
    float enFrac = (player.MaxEnergy() > 0)
        ? (float)(player.getEnergy() / player.MaxEnergy()) : 0.f;
    if (enFrac < 0.f) enFrac = 0.f;
    if (enFrac > 1.f) enFrac = 1.f;
    std::string enLabel = "EN " + std::to_string((int)player.getEnergy())
                        + "/" + std::to_string((int)player.MaxEnergy());
    drawBar(x + barW + 16, y, barW, barH, enFrac, C_EN_BG, C_EN, enLabel);

    // Level + name
    std::string lvl = "LVL " + std::to_string(player.getLevel())
                    + "   " + player.getName();
    DrawText(lvl.c_str(), x + barW*2 + 32, y + 2, FS, C_ACCENT);

    // Combat flash
    if (player.getInCombat()) {
        int cx = SW - PAD - 130;
        DrawRectangle(cx - 6, y - 2, 136, barH + 4, C_COMBAT);
        DrawText("[ IN COMBAT ]", cx, y + 2, FS_SMALL, BLACK);
    }

    // Stats row
    y += barH + 14;
    std::string stats =
        "STR " + std::to_string((int)player.getStats().strength)  + "   " +
        "DEX " + std::to_string((int)player.getStats().dexterity) + "   " +
        "INT " + std::to_string((int)player.getStats().intelligence) + "   " +
        "DEF " + std::to_string((int)player.getStats().defence);
    DrawText(stats.c_str(), x, y, FS_SMALL, C_DIM);

    // EXP
    y += LINE_H - 4;
}

// -----------------------------------------------------------------------
// drawRoom
// -----------------------------------------------------------------------
static void drawRoom(const Room* room, SpriteAnimator& thomas) {
    if (!room) return;

    drawPanel(ROOM_PANEL, C_PANEL, C_BORDER);

    int x  = (int)ROOM_PANEL.x + PAD;
    int y  = (int)ROOM_PANEL.y + PAD;
    int mW = (int)ROOM_PANEL.width - PAD * 2;

    // Room name
    DrawText(room->getName().c_str(), x, y, FS_TITLE, C_ACCENT);
    y += FS_TITLE + 6;
    DrawLine(x, y, x + mW, y, C_BORDER);
    y += 10;

    // Scene viewport: tile background, animated props, NPCs, Thomas
    Rectangle sceneRect = { (float)x, (float)y, (float)mW, (float)SCENE_H };
    DrawRectangleRec(sceneRect, BLACK);
    g_scene.drawFloor(room->getName(), x, y, TILE_SCALE);
    g_scene.drawProps(room->getName(), x, y, mW, SCENE_H, TILE_SCALE);
    g_scene.drawNpcs(room->getNpcEntities(), x, y, mW, SCENE_H, TILE_SCALE);
    thomas.draw(x + mW / 2, y + (int)(SCENE_H * 0.65f), 2.0f);
    DrawRectangleLinesEx(sceneRect, 1.5f, C_BORDER);
    y += SCENE_H + 10;

    // Description
    y = drawWrapped(room->getDescription(), x, y, mW, FS, C_TEXT, 3);
    y += 8;

    // Two-column layout: NPCs + Items left, Exits right
    int colW  = mW / 2 - 8;
    int leftX = x;
    int rightX = x + colW + 16;
    int leftY  = y;
    int rightY = y;

    // NPCs
    if (!room->getNpcEntities().empty()) {
        DrawText("Characters", leftX, leftY, FS_SMALL, C_DIM);
        leftY += LINE_H - 2;
        for (const auto& npc : room->getNpcEntities()) {
            std::string line = "  * " + npc->getName();
            DrawText(line.c_str(), leftX, leftY, FS, C_NPC);
            leftY += LINE_H;
        }
        leftY += 6;
    }

    // Items
    if (!room->getItems().empty()) {
        DrawText("Items", leftX, leftY, FS_SMALL, C_DIM);
        leftY += LINE_H - 2;
        for (const auto& item : room->getItems()) {
            std::string line = "  + " + item->getName();
            DrawText(line.c_str(), leftX, leftY, FS, C_ITEM);
            leftY += LINE_H;
        }
    }

    // Exits
    DrawText("Exits", rightX, rightY, FS_SMALL, C_DIM);
    rightY += LINE_H - 2;
    for (const auto& e : room->getExits()) {
        std::string line = "  " + e.first + " -> " + e.second->getName();
        DrawText(line.c_str(), rightX, rightY, FS, C_EXIT);
        rightY += LINE_H;
    }

    // Divider between columns
    int divX = x + colW + 8;
    DrawLine(divX, y, divX, (int)ROOM_PANEL.y + (int)ROOM_PANEL.height - PAD, C_BORDER);
}

// -----------------------------------------------------------------------
// drawInputBar
// -----------------------------------------------------------------------
static void drawInputBar(const std::string& inputBuffer) {
    drawPanel(INPUT_PANEL, C_INPUT_BG, C_BORDER);

    int x = (int)INPUT_PANEL.x + PAD;
    int y = (int)INPUT_PANEL.y + 10;

    DrawText("Command:", x, y, FS_SMALL, C_DIM);

    // blinking cursor — visible for 30 frames, hidden for 30
    bool cursorOn = (GetTime() * 2.0 - (int)(GetTime() * 2.0)) < 0.5;
    std::string prompt = "> " + inputBuffer + (cursorOn ? "|" : " ");
    DrawText(prompt.c_str(), x + 80, y, FS, C_INPUT_TEXT);

    // hint
    DrawText("Type a command and press Enter.  'help' for a list.",
             x, y + LINE_H, FS_SMALL, C_DIM);
}
static void drawPortrait(const Player& player, SpriteAnimator& animator) {
    Rectangle panel = {624, 16, 160, 200};
    DrawRectangleRec(panel, {25, 25, 35, 255});
    DrawRectangleLinesEx(panel, 1.5f, {60, 60, 90, 255});
    DrawText(player.getName().c_str(), 632, 24, 14, {180, 140, 60, 255});
    animator.draw(704, 120, 2.5f);
    const char* label = player.getInCombat() ? "ATTACKING" :
                        player.getHealth() <= 0 ? "DEFEATED" : "IDLE";
    DrawText(label, 632, 190, 13, {100, 100, 120, 255});
}
// -----------------------------------------------------------------------
// drawGame — entry point called every frame from main.cpp
// -----------------------------------------------------------------------
void drawGame(const World& world, const Player& player,
              const std::string& inputBuffer, SpriteAnimator& animator) {
    ensureSceneLoaded(world);
    g_scene.update(GetFrameTime());

    BeginDrawing();
    ClearBackground(C_BG);
    drawRoom(player.getLocation(), animator);
    drawHUD(player);
    drawPortrait(player, animator);  // add this line
    drawInputBar(inputBuffer);
    EndDrawing();
}