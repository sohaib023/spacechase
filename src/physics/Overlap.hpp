#pragma once

// Narrow-phase overlap tests. `Manifold.normal` points from B toward A — i.e.
// the direction to push A out of B — with `penetration` the overlap depth.

#include "physics/Shapes.hpp"

#include <glm/glm.hpp>

namespace sc {

struct Manifold {
    bool hit = false;
    glm::vec2 normal{0.0f, 0.0f};
    float penetration = 0.0f;
};

// The load-bearing case for ship (circle) vs wall (AABB).
Manifold circle_vs_aabb(const Circle& a, const AABB& b);
Manifold circle_vs_circle(const Circle& a, const Circle& b);
bool aabb_vs_aabb(const AABB& a, const AABB& b);

} // namespace sc
