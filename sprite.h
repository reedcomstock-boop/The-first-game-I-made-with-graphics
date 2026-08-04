#ifndef SPRITE_H
#define SPRITE_H

#include "raylib.h"
#include <string>

// Which animation is currently playing
enum class AnimState {
    IdleDown, IdleLeft, IdleRight, IdleUp,
    RunDown,  RunLeft,  RunRight,  RunUp,
    AttackDown, AttackLeft, AttackRight, AttackUp,
    DeathDown,  DeathLeft,  DeathRight,  DeathUp
};

class SpriteAnimator {
public:
    SpriteAnimator();
    ~SpriteAnimator();

    void load(const std::string& assetDir);  // loads all 16 sheets
    void unload();

    void setState(AnimState state);
    AnimState getState() const { return currentState; }

    // Call once per frame — advances animation
    void update(float dt);

    // Draw the current frame centered at (x, y) scaled up by scale
    void draw(int x, int y, float scale = 2.0f) const;

    bool isDeathFinished() const;  // true when death anim reached last frame

private:
    static const int NUM_ANIMS = 16;

    struct AnimSheet {
        Texture2D texture;
        int       frameCount;
        float     frameDuration;  // seconds per frame
    };

    AnimSheet sheets[NUM_ANIMS];
    AnimState currentState;
    int       currentFrame;
    float     frameTimer;
    bool      loaded;

    int       stateIndex() const;
    AnimSheet& currentSheet();
    const AnimSheet& currentSheet() const;
};

#endif // SPRITE_H