#include "sprite.h"
#include <string>

static const char* SHEET_FILES[16] = {
    "idle_down_40x40.png",    "idle_left_40x40.png",
    "idle_right_40x40.png",   "idle_up_40x40.png",
    "run_down_40x40.png",     "run_left_40x40.png",
    "run_right_40x40.png",    "run_up_40x40.png",
    "attack_down_40x40.png",  "attack_left_40x40.png",
    "attack_right_40x40.png", "attack_up_40x40.png",
    "death_down_40x40.png",   "death_left_40x40.png",
    "death_right_40x40.png",  "death_up_40x40.png",
};
static const int   SHEET_FRAMES[16] = {4,4,4,4, 6,6,6,6, 7,7,7,7, 9,9,9,9};
static const float SHEET_SPF[16]    = {
    0.18f,0.18f,0.18f,0.18f,
    0.10f,0.10f,0.10f,0.10f,
    0.08f,0.08f,0.08f,0.08f,
    0.12f,0.12f,0.12f,0.12f
};

SpriteAnimator::SpriteAnimator()
    : currentState(AnimState::IdleDown), currentFrame(0),
      frameTimer(0.0f), loaded(false) {}

SpriteAnimator::~SpriteAnimator() { unload(); }

void SpriteAnimator::load(const std::string& assetDir) {
    for (int i = 0; i < NUM_ANIMS; i++) {
        std::string path = assetDir + "/" + SHEET_FILES[i];
        sheets[i].texture       = LoadTexture(path.c_str());
        sheets[i].frameCount    = SHEET_FRAMES[i];
        sheets[i].frameDuration = SHEET_SPF[i];
    }
    loaded = true;
}

void SpriteAnimator::unload() {
    if (!loaded) return;
    for (int i = 0; i < NUM_ANIMS; i++) UnloadTexture(sheets[i].texture);
    loaded = false;
}

int SpriteAnimator::stateIndex() const { return static_cast<int>(currentState); }
SpriteAnimator::AnimSheet& SpriteAnimator::currentSheet() { return sheets[stateIndex()]; }
const SpriteAnimator::AnimSheet& SpriteAnimator::currentSheet() const { return sheets[stateIndex()]; }

void SpriteAnimator::setState(AnimState state) {
    if (state == currentState) return;
    // Never interrupt a death animation mid-play
    if (currentState == AnimState::DeathDown  || currentState == AnimState::DeathLeft  ||
        currentState == AnimState::DeathRight || currentState == AnimState::DeathUp) {
        if (!isDeathFinished()) return;
    }
    currentState = state;
    currentFrame = 0;
    frameTimer   = 0.0f;
}

void SpriteAnimator::update(float dt) {
    if (!loaded) return;
    const AnimSheet& sheet = currentSheet();
    frameTimer += dt;
    if (frameTimer >= sheet.frameDuration) {
        frameTimer -= sheet.frameDuration;
        bool looping = (currentState < AnimState::AttackDown);
        if (currentFrame < sheet.frameCount - 1)
            currentFrame++;
        else if (looping)
            currentFrame = 0;
        // attack and death hold last frame when done
    }
}

void SpriteAnimator::draw(int x, int y, float scale) const {
    if (!loaded) return;
    const AnimSheet& sheet = currentSheet();
    int sw = 40;
    int sh = 40;
    Rectangle src = { (float)(currentFrame * sw), 0.0f, (float)sw, (float)sh };
    Rectangle dst = {
        (float)(x - (int)(sw * scale * 0.5f)),
        (float)(y - (int)(sh * scale * 0.5f)),
        sw * scale,
        sh * scale
    };
    DrawTexturePro(sheet.texture, src, dst, {0, 0}, 0.0f, WHITE);
}

bool SpriteAnimator::isDeathFinished() const {
    return currentFrame >= currentSheet().frameCount - 1;
}