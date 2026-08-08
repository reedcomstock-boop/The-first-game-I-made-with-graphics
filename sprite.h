#ifndef SPRITE_H
#define SPRITE_H

#include "raylib.h"
#include <string>

// Which animation is currently playing
enum class AnimState {
    IdleDown, IdleLeft, IdleRight, IdleUp,
    RunDown,  RunLeft,  RunRight,  RunUp,
    Attack, Death
};

class SpriteAnimator {
public:
    SpriteAnimator();
    ~SpriteAnimator();

    // walkPath: tommy_walking.png   (48x48 cells, 3 cols x 4 rows: Down,Left,Right,Up)
    // deathPath: tommy_hurt.png     (48x48 cells, 3 frames, Down-facing only)
    // attackPath: tommy_in_battle.png (64x64 cells; reads row 2, cols 3-5 — the "swing" motion)
    void load(const std::string& walkPath, const std::string& deathPath, const std::string& attackPath);
    void unload();

    void setState(AnimState state);
    AnimState getState() const { return currentState; }

    void update(float dt);
    void draw(int x, int y, float scale = 2.0f) const;

    bool isDeathFinished() const;
    bool isAttackFinished() const;

private:
    static const int WALK_FRAME_SIZE   = 48;
    static const int ATTACK_FRAME_SIZE = 64;
    static const int WALK_COLS    = 3;
    static const int DEATH_FRAMES = 3;
    static const int ATTACK_FRAMES = 3;
    static const int ATTACK_ROW    = 2;  // "swing" motion row in the SV grid
    static const int ATTACK_COL0   = 3;  // "swing" starts at column 3

    Texture2D walkTexture;
    Texture2D deathTexture;
    Texture2D attackTexture;

    AnimState currentState;
    int       currentFrame;
    float     frameTimer;
    bool      loaded;

    float frameDuration(AnimState state) const;
    int   frameCount(AnimState state) const;
    int   rowFor(AnimState state) const;
};
#endif // SPRITE_H