#pragma once

// Collision shapes in world meters. POD, header-only, glm-only.

#include <glm/glm.hpp>

#include <cmath>

namespace sc {

inline constexpr int kMaxHitboxVerts = 8;

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

// A convex polygon in world space (vertices in order). The collision narrow-phase
// converts AABB/polygon hitboxes to this for SAT. Circles are handled separately.
struct Convex {
    glm::vec2 verts[kMaxHitboxVerts]{};
    int count = 0;

    glm::vec2 centroid() const {
        glm::vec2 c{0.0f, 0.0f};
        for (int i = 0; i < count; ++i) {
            c += verts[i];
        }
        return count > 0 ? c / static_cast<float>(count) : c;
    }
};

inline glm::vec2 rotate_point(glm::vec2 p, float cosR, float sinR) {
    return {p.x * cosR - p.y * sinR, p.x * sinR + p.y * cosR};
}

} // namespace sc
