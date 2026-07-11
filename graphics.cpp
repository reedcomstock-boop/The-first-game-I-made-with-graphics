#include "graphics.h"
#include "raylib.h"
#include <string>

// Layout constants — change these to adjust the UI
static const int SCREEN_W     = 800;
static const int SCREEN_H     = 600;
static const int FONT_SIZE    = 18;
static const int LINE_H       = 24;   // vertical spacing between lines
static const int PAD          = 20;   // left margin padding

// Colours
static const Color COL_BG       = BLACK;
static const Color COL_TITLE    = YELLOW;
static const Color COL_TEXT     = WHITE;
static const Color COL_DIM      = GRAY;
static const Color COL_INPUT    = GREEN;
static const Color COL_HEALTH   = RED;
static const Color COL_ENERGY   = SKYBLUE;
static const Color COL_COMBAT   = ORANGE;
static const Color COL_BORDER   = DARKGRAY;

// -----------------------------------------------------------------------
// drawHUD — bottom strip showing player vitals
// -----------------------------------------------------------------------
static void drawHUD(const Player& player) {
    int hudY = SCREEN_H - 80;

    // Divider line
    DrawLine(0, hudY, SCREEN_W, hudY, COL_BORDER);

    // Health
    std::string hp = "HP: " + std::to_string((int)player.getHealth())
                   + " / "  + std::to_string((int)player.MaxHealth());
    DrawText(hp.c_str(), PAD, hudY + 10, FONT_SIZE, COL_HEALTH);

    // Energy
    std::string en = "EN: " + std::to_string((int)player.getEnergy())
                   + " / "  + std::to_string((int)player.MaxEnergy());
    DrawText(en.c_str(), PAD + 200, hudY + 10, FONT_SIZE, COL_ENERGY);

    // Level
    std::string lv = "LVL: " + std::to_string(player.getLevel());
    DrawText(lv.c_str(), PAD + 400, hudY + 10, FONT_SIZE, COL_TEXT);

    // Combat warning
    if (player.getInCombat()) {
        DrawText("[ IN COMBAT ]", PAD + 560, hudY + 10, FONT_SIZE, COL_COMBAT);
    }

    // Stats row
    std::string stats =
        "STR:" + std::to_string((int)player.getStats().strength)  + "  " +
        "DEX:" + std::to_string((int)player.getStats().dexterity) + "  " +
        "INT:" + std::to_string((int)player.getStats().intelligence) + "  " +
        "DEF:" + std::to_string((int)player.getStats().defence);
    DrawText(stats.c_str(), PAD, hudY + 38, FONT_SIZE - 2, COL_DIM);
}

// -----------------------------------------------------------------------
// drawRoom — centre panel showing current room state
// -----------------------------------------------------------------------
static void drawRoom(const Room* room) {
    if (!room) return;

    int y = PAD;

    // Room name
    DrawText(room->getName().c_str(), PAD, y, FONT_SIZE + 4, COL_TITLE);
    y += LINE_H + 8;

    // Divider
    DrawLine(PAD, y, SCREEN_W - PAD, y, COL_BORDER);
    y += 10;

    // Description — word-wrap at ~60 chars manually kept short for now
    std::string desc = room->getDescription();
    // Draw up to 3 lines of ~70 chars each (simple truncation for now)
    int charsPerLine = 70;
    int linesDrawn = 0;
    size_t pos = 0;
    while (pos < desc.size() && linesDrawn < 4) {
        std::string line = desc.substr(pos, charsPerLine);
        // try to break at a space
        if (pos + charsPerLine < desc.size()) {
            size_t lastSpace = line.rfind(' ');
            if (lastSpace != std::string::npos) {
                line = line.substr(0, lastSpace);
            }
        }
        DrawText(line.c_str(), PAD, y, FONT_SIZE - 2, COL_TEXT);
        pos += line.size() + 1;
        y += LINE_H;
        linesDrawn++;
    }
    y += 10;

    // NPCs
    if (!room->getNpcEntities().empty()) {
        DrawText("Characters here:", PAD, y, FONT_SIZE, COL_DIM);
        y += LINE_H;
        for (const auto& npc : room->getNpcEntities()) {
            std::string npcLine = "  > " + npc->getName();
            DrawText(npcLine.c_str(), PAD, y, FONT_SIZE, COL_TEXT);
            y += LINE_H;
        }
        y += 6;
    }

    // Items on the ground
    if (!room->getItems().empty()) {
        DrawText("Items here:", PAD, y, FONT_SIZE, COL_DIM);
        y += LINE_H;
        for (const auto& item : room->getItems()) {
            std::string itemLine = "  > " + item->getName();
            DrawText(itemLine.c_str(), PAD, y, FONT_SIZE, COL_TEXT);
            y += LINE_H;
        }
        y += 6;
    }

    // Exits
    DrawText("Exits:", PAD, y, FONT_SIZE, COL_DIM);
    y += LINE_H;
    for (const auto& e : room->getExits()) {
        std::string exitLine = "  > " + e.first + "  {" + e.second->getName() + "}";
        DrawText(exitLine.c_str(), PAD, y, FONT_SIZE, COL_TEXT);
        y += LINE_H;
    }
}

// -----------------------------------------------------------------------
// drawInputBar — shows what the player is currently typing
// -----------------------------------------------------------------------
static void drawInputBar(const std::string& inputBuffer) {
    int barY = SCREEN_H - 30;
    DrawLine(0, barY - 4, SCREEN_W, barY - 4, COL_BORDER);
    std::string prompt = "> " + inputBuffer + "_";
    DrawText(prompt.c_str(), PAD, barY, FONT_SIZE, COL_INPUT);
}

// -----------------------------------------------------------------------
// drawGame — called once per frame from main.cpp
// -----------------------------------------------------------------------
void drawGame(const World& world, const Player& player,
              const std::string& inputBuffer) {
    (void)world; // world available for future panels (minimap, etc.)

    BeginDrawing();
    ClearBackground(COL_BG);

    drawRoom(player.getLocation());
    drawHUD(player);
    drawInputBar(inputBuffer);

    EndDrawing();
}