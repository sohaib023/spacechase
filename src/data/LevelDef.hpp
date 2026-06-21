#pragma once

// Parsed level data (the M3 subset of the eventual editor level format, brief
// §7): the floor/void geometry, the player spawn, and wall instances that
// reference wall definitions by id. App instantiates the walls (resolving
// textures); this struct stays GPU-free so LevelLoader is unit-testable.

#include "gameplay/Level.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace sc {

struct WallInstance {
    std::string def;
    glm::vec2 center{0.0f, 0.0f};
    glm::vec2 halfExtents{0.0f, 0.0f};
    bool hasHalfExtents = false; // false → use the WallDef's default size
};

struct LevelData {
    Level level;                       // floor + void rects
    glm::vec2 playerSpawn{0.0f, 0.0f};
    std::vector<WallInstance> walls;
};

} // namespace sc
