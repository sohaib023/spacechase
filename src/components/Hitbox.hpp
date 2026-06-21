#pragma once

// Collision shape attached to an entity, in meters, relative to its Transform.
// Ships use Circle; walls use AABB.

#include <glm/glm.hpp>

namespace sc {

enum class HitboxShape { Circle, AABB };

struct Hitbox {
    HitboxShape shape = HitboxShape::Circle;
    float radius = 0.5f;                 // Circle
    glm::vec2 halfExtents{0.5f, 0.5f};   // AABB
    glm::vec2 offset{0.0f, 0.0f};        // local offset from Transform.position
};

} // namespace sc
