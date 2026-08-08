#include "sprite.h"

static const int WALK_SEQUENCE[4] = {1, 0, 1, 2}; // standing, left-step, standing, right-step

SpriteAnimator::SpriteAnimator()
    : currentState(AnimState::IdleDown), currentFrame(0),
      frameTimer(0.0f), loaded(false) {}

SpriteAnimator::~SpriteAnimator() { unload(); }

void SpriteAnimator::load(const std::string& walkPath, const std::string& deathPath,
                           const std::string& attackPath) {
    walkTexture   = LoadTexture(walkPath.c_str());
    deathTexture  = LoadTexture(deathPath.c_str());
    attackTexture = LoadTexture(attackPath.c_str());

    if (walkTexture.id == 0)   TraceLog(LOG_WARNING, "SpriteAnimator: failed to load walk sheet: %s", walkPath.c_str());
    if (deathTexture.id == 0)  TraceLog(LOG_WARNING, "SpriteAnimator: failed to load death sheet: %s", deathPath.c_str());
    if (attackTexture.id == 0) TraceLog(LOG_WARNING, "SpriteAnimator: failed to load attack sheet: %s", attackPath.c_str());

    loaded = true;
}

void SpriteAnimator::unload() {
    if (!loaded) return;
    UnloadTexture(walkTexture);
    UnloadTexture(deathTexture);
    UnloadTexture(attackTexture);
    loaded = false;
}

float SpriteAnimator::frameDuration(AnimState state) const {
    if (state == AnimState::Death)  return 0.15f;
    if (state == AnimState::Attack) return 0.10f;
    return 0.12f;
}

int SpriteAnimator::frameCount(AnimState state) const {
    if (state == AnimState::Death)  return DEATH_FRAMES;
    if (state == AnimState::Attack) return ATTACK_FRAMES;
    return 4; // length of WALK_SEQUENCE
}

int SpriteAnimator::rowFor(AnimState state) const {
    switch (state) {
        case AnimState::IdleDown:  case AnimState::RunDown:  return 0;
        case AnimState::IdleLeft:  case AnimState::RunLeft:  return 1;
        case AnimState::IdleRight: case AnimState::RunRight: return 2;
        case AnimState::IdleUp:    case AnimState::RunUp:    return 3;
        default: return 0;
    }
}

void SpriteAnimator::setState(AnimState state) {
    if (state == currentState) return;
    if (currentState == AnimState::Death  && !isDeathFinished())  return;
    if (currentState == AnimState::Attack && !isAttackFinished()) return;
    currentState = state;
    currentFrame = 0;
    frameTimer   = 0.0f;
}

void SpriteAnimator::update(float dt) {
    if (!loaded) return;
    frameTimer += dt;
    float dur = frameDuration(currentState);
    if (frameTimer >= dur) {
        frameTimer -= dur;
        int count = frameCount(currentState);
        bool looping = (currentState != AnimState::Death && currentState != AnimState::Attack);
        if (currentFrame < count - 1)
            currentFrame++;
        else if (looping)
            currentFrame = 0;
        // death and attack hold on their last frame when finished
    }
}

void SpriteAnimator::draw(int x, int y, float scale) const {
    if (!loaded) return;

    Texture2D tex;
    Rectangle src;
    int size;

    if (currentState == AnimState::Death) {
        tex  = deathTexture;
        size = WALK_FRAME_SIZE;
        src  = { (float)(currentFrame * size), 0.0f, (float)size, (float)size };
    } else if (currentState == AnimState::Attack) {
        tex  = attackTexture;
        size = ATTACK_FRAME_SIZE;
        int col = ATTACK_COL0 + currentFrame;
        src  = { (float)(col * size), (float)(ATTACK_ROW * size), (float)size, (float)size };
    } else {
        tex  = walkTexture;
        size = WALK_FRAME_SIZE;
        int col = WALK_SEQUENCE[currentFrame];
        int row = rowFor(currentState);
        src  = { (float)(col * size), (float)(row * size), (float)size, (float)size };
    }

    if (tex.id == 0) return;
float displayScale = scale * ((float)WALK_FRAME_SIZE / (float)size); // keeps attack visually same size as walk

    Rectangle dst = {
        (float)(x - (int)(size * displayScale * 0.5f)),
        (float)(y - (int)(size * displayScale * 0.5f)),
        size * displayScale,
        size * displayScale
    };
    DrawTexturePro(tex, src, dst, {0, 0}, 0.0f, WHITE);
}

bool SpriteAnimator::isDeathFinished() const {
    return currentState == AnimState::Death && currentFrame >= DEATH_FRAMES - 1;
}

bool SpriteAnimator::isAttackFinished() const {
    return currentState == AnimState::Attack && currentFrame >= ATTACK_FRAMES - 1;
}