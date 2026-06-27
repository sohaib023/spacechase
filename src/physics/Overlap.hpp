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

Manifold circle_vs_aabb(const Circle& a, const AABB& b);
Manifold circle_vs_circle(const Circle& a, const Circle& b);
bool aabb_vs_aabb(const AABB& a, const AABB& b);

// Convex-polygon collision via SAT. Returned `normal` pushes A out of B; the ship
// (a polygon) vs walls (AABBs treated as convex) is the load-bearing case now.
Manifold convex_vs_convex(const Convex& a, const Convex& b);
// SAT for a circle vs a convex polygon. `normal` pushes the CIRCLE out of the polygon.
Manifold circle_vs_convex(const Circle& circle, const Convex& poly);

} // namespace sc
