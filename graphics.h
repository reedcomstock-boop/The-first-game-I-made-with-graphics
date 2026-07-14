#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <string>
#include "world.h"
#include "sprite.h"

#include "player.h"
void drawGame(const World& world, const Player& player, const std::string& inputBuffer, SpriteAnimator& animator);
void unloadSceneAssets(); // call once before CloseWindow() to free tile/NPC/prop textures

#endif // GRAPHICS_H