#pragma once

// Collision shape attached to an entity, in meters, relative to its Transform.
// Ships use Circle; walls use AABB.

#include "physics/Shapes.hpp" // kMaxHitboxVerts

#include <glm/glm.hpp>

namespace sc {

enum class HitboxShape { Circle, AABB, Polygon };

struct Hitbox {
    HitboxShape shape = HitboxShape::Circle;
    float radius = 0.5f;                 // Circle
    glm::vec2 halfExtents{0.5f, 0.5f};   // AABB
    glm::vec2 offset{0.0f, 0.0f};        // local offset from Transform.position
    // Polygon: convex local vertices (rotated by the entity's Transform.rotation).
    // Fixed-capacity so the component stays trivially copyable / serializable.
    int vertexCount = 0;
    glm::vec2 vertices[kMaxHitboxVerts]{};
};

} // namespace sc
