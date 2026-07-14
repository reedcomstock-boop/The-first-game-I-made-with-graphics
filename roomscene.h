#ifndef ROOMSCENE_H
#define ROOMSCENE_H

#include "raylib.h"
#include "tileset.h"
#include "strip_animator.h"
#include "world.h"
#include <string>
#include <vector>
#include <unordered_map>

// One cell of the background layer: which tileset (by registry index) and
// which tile within it. tilesetId == -1 means "draw nothing".
struct TileRef {
    int tilesetId;
    int tileIndex;
};

// A themed prop placed in a room (bonfire, forge). Position is stored as a
// fraction of the scene viewport so it doesn't need updating if the
// viewport size ever changes.
struct PropInstance {
    std::string clipName; // key into propAnimators
    float relX, relY;     // 0..1 within the scene viewport
};

struct RoomScene {
    std::vector<std::vector<TileRef>> floor; // [row][col]
    std::vector<PropInstance> props;
};

// Everything rendering needs to draw the "animated scene view" for a room:
// tile background, NPCs at their positions, and animated props. Lives
// entirely in the rendering layer — GameLoop and World never touch this.
class RoomSceneManager {
public:
    RoomSceneManager();
    ~RoomSceneManager();

    void loadTilesets(const std::string& assetDir);
    void loadNpcSprites(const std::string& assetDir);
    void loadProps(const std::string& assetDir);
    void loadMonsterSprites(const std::string& assetDir);

    // Builds a default floor+wall layout (with a couple of themed
    // overrides — water for The Lake, dungeon tiles for maze rooms) for
    // every room in the world, sized to a cols x rows tile grid.
    void buildLayouts(const World& world, int cols, int rows);

    void update(float dt);

    // Frees all tile/NPC/prop textures. Must be called before CloseWindow() —
    // calling UnloadTexture() after the GL context is gone (which is what
    // happens if this only runs in the destructor, since g_scene is static
    // and destructs after main() returns) will crash.
    void unloadAll();

    void drawFloor(const std::string& roomName, int originX, int originY, float scale) const;
    void drawProps(const std::string& roomName, int originX, int originY,
                    int viewportW, int viewportH, float scale) const;
    void drawNpcs(const std::vector<NPC*>& npcsInRoom, int originX, int originY,
                   int viewportW, int viewportH, float scale) const;

private:
    std::vector<TileSet> tilesets;
    std::unordered_map<std::string, int> tilesetIndex; // name -> index into tilesets

    std::unordered_map<std::string, RoomScene> rooms; // room name -> layout
    std::unordered_map<std::string, StripAnimator> npcAnimators;  // npc name -> animator
    std::unordered_map<std::string, StripAnimator> propAnimators; // clip name -> animator

    int registerTileset(const std::string& name, const std::string& path);
};

#endif // ROOMSCENE_H