#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <string>
#include "world.h"
#include "player.h"
void drawGame(const World& world, const Player& player, const std::string& inputBuffer);

#endif // GRAPHICS_H