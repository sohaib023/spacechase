#pragma once

// Collision shapes in world meters. POD, header-only, glm-only.

#include <glm/glm.hpp>

namespace sc {

struct Circle {
    glm::vec2 center{0.0f, 0.0f};
    float radius = 0.0f;
};

struct AABB {
    glm::vec2 center{0.0f, 0.0f};
    glm::vec2 halfExtents{0.0f, 0.0f};

    glm::vec2 min() const { return center - halfExtents; }
    glm::vec2 max() const { return center + halfExtents; }
};

} // namespace sc
