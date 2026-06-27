#include "physics/Overlap.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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

namespace {

void project_convex(const Convex& c, glm::vec2 axis, float& outMin, float& outMax) {
    outMin = outMax = glm::dot(c.verts[0], axis);
    for (int i = 1; i < c.count; ++i) {
        const float d = glm::dot(c.verts[i], axis);
        outMin = std::min(outMin, d);
        outMax = std::max(outMax, d);
    }
}

// Accumulate the minimum-overlap axis over one polygon's edge normals. Returns
// false if a separating axis is found (no collision).
bool sat_edges(const Convex& poly, const Convex& a, const Convex& b, float& minOverlap,
               glm::vec2& mtvAxis) {
    for (int i = 0; i < poly.count; ++i) {
        const glm::vec2 p1 = poly.verts[i];
        const glm::vec2 p2 = poly.verts[(i + 1) % poly.count];
        const glm::vec2 edge = p2 - p1;
        const float len = std::sqrt(edge.x * edge.x + edge.y * edge.y);
        if (len < 1e-6f) {
            continue;
        }
        const glm::vec2 axis{-edge.y / len, edge.x / len};
        float aMin = 0.0f;
        float aMax = 0.0f;
        float bMin = 0.0f;
        float bMax = 0.0f;
        project_convex(a, axis, aMin, aMax);
        project_convex(b, axis, bMin, bMax);
        const float overlap = std::min(aMax, bMax) - std::max(aMin, bMin);
        if (overlap <= 0.0f) {
            return false; // separating axis
        }
        if (overlap < minOverlap) {
            minOverlap = overlap;
            mtvAxis = axis;
        }
    }
    return true;
}

} // namespace

Manifold convex_vs_convex(const Convex& a, const Convex& b) {
    Manifold m;
    float minOverlap = std::numeric_limits<float>::max();
    glm::vec2 mtvAxis{0.0f, 0.0f};
    if (!sat_edges(a, a, b, minOverlap, mtvAxis) || !sat_edges(b, a, b, minOverlap, mtvAxis)) {
        return m; // separating axis → no collision
    }
    // Orient the axis so it pushes A out of B.
    if (glm::dot(a.centroid() - b.centroid(), mtvAxis) < 0.0f) {
        mtvAxis = -mtvAxis;
    }
    m.hit = true;
    m.normal = mtvAxis;
    m.penetration = minOverlap;
    return m;
}

Manifold circle_vs_convex(const Circle& circle, const Convex& poly) {
    Manifold m;
    float minOverlap = std::numeric_limits<float>::max();
    glm::vec2 mtvAxis{0.0f, 0.0f};

    auto testAxis = [&](glm::vec2 axis) -> bool {
        const float len = std::sqrt(axis.x * axis.x + axis.y * axis.y);
        if (len < 1e-6f) {
            return true; // skip degenerate
        }
        axis /= len;
        float pMin = 0.0f;
        float pMax = 0.0f;
        project_convex(poly, axis, pMin, pMax);
        const float c = glm::dot(circle.center, axis);
        const float cMin = c - circle.radius;
        const float cMax = c + circle.radius;
        const float overlap = std::min(pMax, cMax) - std::max(pMin, cMin);
        if (overlap <= 0.0f) {
            return false;
        }
        if (overlap < minOverlap) {
            minOverlap = overlap;
            mtvAxis = axis;
        }
        return true;
    };

    // Polygon edge normals.
    for (int i = 0; i < poly.count; ++i) {
        const glm::vec2 p1 = poly.verts[i];
        const glm::vec2 p2 = poly.verts[(i + 1) % poly.count];
        if (!testAxis({-(p2.y - p1.y), p2.x - p1.x})) {
            return m;
        }
    }
    // Axis from the nearest vertex to the circle center.
    int nearest = 0;
    float nearestDist2 = std::numeric_limits<float>::max();
    for (int i = 0; i < poly.count; ++i) {
        const glm::vec2 d = poly.verts[i] - circle.center;
        const float dist2 = d.x * d.x + d.y * d.y;
        if (dist2 < nearestDist2) {
            nearestDist2 = dist2;
            nearest = i;
        }
    }
    if (!testAxis(circle.center - poly.verts[nearest])) {
        return m;
    }

    // Orient the axis so it pushes the circle out of the polygon.
    if (glm::dot(circle.center - poly.centroid(), mtvAxis) < 0.0f) {
        mtvAxis = -mtvAxis;
    }
    m.hit = true;
    m.normal = mtvAxis;
    m.penetration = minOverlap;
    return m;
}

} // namespace sc
