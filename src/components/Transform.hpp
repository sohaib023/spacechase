#pragma once

// World-space placement. The single spatial source of truth for an entity.

#include <glm/glm.hpp>

namespace sc {

struct Transform {
    glm::vec2 position{0.0f, 0.0f};
    float rotation = 0.0f; // radians; 0 = pointing +Y (up)
};

} // namespace sc
