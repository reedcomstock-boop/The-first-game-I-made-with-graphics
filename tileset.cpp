#include "tileset.h"

TileSet::TileSet() : tileSize(16), columns(0), rows(0), loaded(false) {}

TileSet::~TileSet() { unload(); }

TileSet::TileSet(TileSet&& other) noexcept
    : texture(other.texture), tileSize(other.tileSize), columns(other.columns), rows(other.rows), loaded(other.loaded) {
    other.texture = Texture{};
    other.tileSize = 16;
    other.columns = 0;
    other.rows = 0;
    other.loaded = false;
}

TileSet& TileSet::operator=(TileSet&& other) noexcept {
    if (this != &other) {
        unload();
        texture = other.texture;
        tileSize = other.tileSize;
        columns = other.columns;
        rows = other.rows;
        loaded = other.loaded;

        other.texture = Texture{};
        other.tileSize = 16;
        other.columns = 0;
        other.rows = 0;
        other.loaded = false;
    }
    return *this;
}

bool TileSet::load(const std::string& path, int size) {
    texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        TraceLog(LOG_WARNING, "TileSet failed to load: %s", path.c_str());
        loaded = false;
        return false;
    }
    tileSize = size;
    columns  = texture.width  / tileSize;
    rows     = texture.height / tileSize;
    loaded   = true;
    TraceLog(LOG_INFO, "TileSet loaded: %s (%d x %d px, %d cols x %d rows)",
             path.c_str(), texture.width, texture.height, columns, rows);
    return true;
}

void TileSet::unload() {
    if (loaded) UnloadTexture(texture);
    loaded = false;
}
void TileSet::drawRegion(int col, int row, int cellsWide, int cellsHigh,
                          float destX, float destY, float scale) const {
    if (!loaded) return;
    Rectangle src = { (float)(col * tileSize), (float)(row * tileSize),
                       (float)(cellsWide * tileSize), (float)(cellsHigh * tileSize) };
    Rectangle dst = { destX, destY,
                       cellsWide * tileSize * scale, cellsHigh * tileSize * scale };
    DrawTexturePro(texture, src, dst, {0, 0}, 0.0f, WHITE);
}
bool TileSet::isLoaded() const { return loaded; }
int  TileSet::getColumns() const { return columns; }
int  TileSet::getRows() const { return rows; }
int  TileSet::getTileSize() const { return tileSize; }

void TileSet::drawTile(int col, int row, float destX, float destY, float scale) const {
    if (!loaded) return;
    Rectangle src = { (float)(col * tileSize), (float)(row * tileSize),
                       (float)tileSize, (float)tileSize };
    Rectangle dst = { destX, destY, tileSize * scale, tileSize * scale };
    DrawTexturePro(texture, src, dst, {0, 0}, 0.0f, WHITE);
}

void TileSet::drawTile(int index, float destX, float destY, float scale) const {
    if (!loaded || columns == 0) return;
    drawTile(index % columns, index / columns, destX, destY, scale);
}