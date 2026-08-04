#ifndef TILESET_H
#define TILESET_H

#include "raylib.h"
#include <string>

// Wraps one tileset PNG sliced into a fixed-size grid (e.g. Floors_Tiles.png,
// 16x16 tiles). Reused for every tileset — Floors, Walls, Water, Dungeon.
class TileSet {
public:
    TileSet();
    ~TileSet();
    TileSet(const TileSet&) = delete;
    TileSet& operator=(const TileSet&) = delete;
    TileSet(TileSet&& other) noexcept;
    TileSet& operator=(TileSet&& other) noexcept;

    void drawRegion(int col, int row, int cellsWide, int cellsHigh, float destX, float destY, float scale) const;
    bool load(const std::string& path, int tileSize = 16);
    void unload();
    bool isLoaded() const;

    int getColumns() const;
    int getRows() const;
    int getTileSize() const;

    void drawTile(int col, int row, float destX, float destY, float scale) const;
    void drawTile(int index, float destX, float destY, float scale) const; // index = row*columns+col

private:
    Texture2D texture;
    int tileSize;
    int columns;
    int rows;
    bool loaded;
};

#endif // TILESET_H