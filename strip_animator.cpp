#include "strip_animator.h"

StripAnimator::StripAnimator()
    : currentClip(-1), currentFrame(0), frameTimer(0.0f) {}

StripAnimator::~StripAnimator() { unload(); }

void StripAnimator::addClip(const std::string& name, const std::string& path,
                             int frameCount, float frameDuration, int frameW, int frameH) {
    AnimClip clip;
    clip.name          = name;
    clip.texture       = LoadTexture(path.c_str());
    clip.frameCount    = frameCount;
    clip.frameDuration = frameDuration;
    clip.frameW        = frameW;
    clip.frameH        = frameH;

    if (clip.texture.id == 0) {
        TraceLog(LOG_WARNING, "StripAnimator clip '%s' failed to load: %s",
                 name.c_str(), path.c_str());
    } else {
        TraceLog(LOG_INFO, "StripAnimator clip '%s' loaded: %s (%d x %d px)",
                 name.c_str(), path.c_str(), clip.texture.width, clip.texture.height);
    }

    clips.push_back(clip);
    if (currentClip < 0) currentClip = 0; // default to first clip added
}
void StripAnimator::unload() {
    for (auto& clip : clips) UnloadTexture(clip.texture);
    clips.clear();
    currentClip = -1;
}

int StripAnimator::findClip(const std::string& name) const {
    for (size_t i = 0; i < clips.size(); i++) {
        if (clips[i].name == name) return (int)i;
    }
    return -1;
}

bool StripAnimator::hasClip(const std::string& name) const {
    return findClip(name) >= 0;
}

void StripAnimator::setState(const std::string& name) {
    int idx = findClip(name);
    if (idx < 0 || idx == currentClip) return;
    currentClip  = idx;
    currentFrame = 0;
    frameTimer   = 0.0f;
}

const std::string& StripAnimator::getState() const {
    static const std::string none = "";
    if (currentClip < 0) return none;
    return clips[currentClip].name;
}

void StripAnimator::update(float dt) {
    if (currentClip < 0) return;
    const AnimClip& clip = clips[currentClip];
    frameTimer += dt;
    if (frameTimer >= clip.frameDuration) {
        frameTimer -= clip.frameDuration;
        currentFrame = (currentFrame + 1) % clip.frameCount;
    }
}

void StripAnimator::draw(int x, int y, float scale) const {
    if (currentClip < 0) return;
    const AnimClip& clip = clips[currentClip];
    if (clip.texture.id == 0) return; // failed to load — draw nothing instead of a garbage texture
    Rectangle src = { (float)(currentFrame * clip.frameW), 0.0f,
                       (float)clip.frameW, (float)clip.frameH };
    Rectangle dst = {
        (float)(x - (int)(clip.frameW * scale * 0.5f)),
        (float)(y - (int)(clip.frameH * scale * 0.5f)),
        clip.frameW * scale,
        clip.frameH * scale
    };
    DrawTexturePro(clip.texture, src, dst, {0, 0}, 0.0f, WHITE);
}