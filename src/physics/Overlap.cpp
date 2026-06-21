#include "physics/Overlap.hpp"

#include <algorithm>
#include <cmath>

namespace sc {

Manifold circle_vs_aabb(const Circle& a, const AABB& b) {
    Manifold m;
    const glm::vec2 bmin = b.min();
    const glm::vec2 bmax = b.max();

    // Center inside the box → deep penetration; push out along the nearest face.
    if (a.center.x >= bmin.x && a.center.x <= bmax.x && a.center.y >= bmin.y &&
        a.center.y <= bmax.y) {
        const float left = a.center.x - bmin.x;
        const float right = bmax.x - a.center.x;
        const float down = a.center.y - bmin.y;
        const float up = bmax.y - a.center.y;
        float best = left;
        glm::vec2 normal{-1.0f, 0.0f};
        if (right < best) { best = right; normal = {1.0f, 0.0f}; }
        if (down < best) { best = down; normal = {0.0f, -1.0f}; }
        if (up < best) { best = up; normal = {0.0f, 1.0f}; }
        m.hit = true;
        m.normal = normal;
        m.penetration = best + a.radius;
        return m;
    }

    // Otherwise: distance from the circle center to the closest point on the box.
    const glm::vec2 closest{std::clamp(a.center.x, bmin.x, bmax.x),
                            std::clamp(a.center.y, bmin.y, bmax.y)};
    const glm::vec2 d = a.center - closest;
    const float dist2 = glm::dot(d, d);
    if (dist2 > a.radius * a.radius) {
        return m; // no hit
    }
    const float dist = std::sqrt(dist2);
    m.hit = true;
    m.penetration = a.radius - dist;
    m.normal = dist > 1e-6f ? d / dist : glm::vec2(0.0f, 1.0f);
    return m;
}

Manifold circle_vs_circle(const Circle& a, const Circle& b) {
    Manifold m;
    const glm::vec2 d = a.center - b.center;
    const float r = a.radius + b.radius;
    const float dist2 = glm::dot(d, d);
    if (dist2 > r * r) {
        return m;
    }
    const float dist = std::sqrt(dist2);
    m.hit = true;
    m.penetration = r - dist;
    m.normal = dist > 1e-6f ? d / dist : glm::vec2(0.0f, 1.0f);
    return m;
}

bool aabb_vs_aabb(const AABB& a, const AABB& b) {
    return std::abs(a.center.x - b.center.x) <= (a.halfExtents.x + b.halfExtents.x) &&
           std::abs(a.center.y - b.center.y) <= (a.halfExtents.y + b.halfExtents.y);
}

} // namespace sc
