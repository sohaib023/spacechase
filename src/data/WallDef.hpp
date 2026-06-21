#pragma once

// A wall/obstacle type. Indestructible walls take their size per-instance; a
// destructible (e.g. a crate) carries health and a default size. JSON-free
// header; parsing lives in DefinitionRegistry.cpp.

#include <glm/glm.hpp>

#include <string>

namespace sc {

struct WallDef {
    std::string id;
    std::string sprite;             // path relative to SC_ASSET_DIR
    std::string normal;             // normal map path (optional)
    bool destructible = false;
    float health = 0.0f;            // used when destructible
    glm::vec2 halfExtents{1.0f, 1.0f}; // default size (instances may override)
};

} // namespace sc
