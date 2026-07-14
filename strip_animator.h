#ifndef STRIP_ANIMATOR_H
#define STRIP_ANIMATOR_H

#include "raylib.h"
#include <string>
#include <vector>

// A single named animation clip loaded from one horizontal sprite strip.
struct AnimClip {
    std::string name;          // e.g. "idle", "run"
    Texture2D   texture;
    int         frameCount;
    float       frameDuration; // seconds per frame
    int         frameW;
    int         frameH;
};

// Lightweight animator for NPCs and animated props. SpriteAnimator assumes a
// fixed 16-sheet 40x40 layout (Thomas only) — this one takes whatever clips
// a character actually has art for, each with its own frame size.
class StripAnimator {
public:
    StripAnimator();
    ~StripAnimator();

    void addClip(const std::string& name, const std::string& path,
                 int frameCount, float frameDuration, int frameW, int frameH);
    void unload();

    void setState(const std::string& name);
    const std::string& getState() const;
    bool hasClip(const std::string& name) const;

    void update(float dt);
    void draw(int x, int y, float scale = 2.0f) const;

private:
    std::vector<AnimClip> clips;
    int   currentClip;
    int   currentFrame;
    float frameTimer;

    int findClip(const std::string& name) const;
};

#endif // STRIP_ANIMATOR_H