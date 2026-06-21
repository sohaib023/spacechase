#pragma once

// Raycasting against collider shapes and the world. Pure shape tests plus a
// registry-level query that returns the nearest hit. Consumers (beams /
// machine-gun) arrive in M5; the API is built and tested now.

#include "ecs/Registry.hpp"
#include "physics/Shapes.hpp"

#include <cstdint>
#include <glm/glm.hpp>

namespace sc {

struct RayHit {
    bool hit = false;
    float t = 0.0f; // distance along (normalized) dir to the hit
    glm::vec2 point{0.0f, 0.0f};
    glm::vec2 normal{0.0f, 0.0f};
    Entity entity = entt::null;
};

// Pure tests. `dir` must be normalized; intersection must lie within [0, maxDist].
// On hit, write the entry distance to `tOut` and the surface normal to `normalOut`.
bool ray_vs_aabb(glm::vec2 origin, glm::vec2 dir, const AABB& box, float maxDist,
                 float& tOut, glm::vec2& normalOut);
bool ray_vs_circle(glm::vec2 origin, glm::vec2 dir, const Circle& circle, float maxDist,
                   float& tOut, glm::vec2& normalOut);

// Nearest collider hit among entities whose layer is in `mask`. `dir` need not
// be normalized (it is normalized internally).
RayHit raycast(const Registry& registry, glm::vec2 origin, glm::vec2 dir, float maxDist,
               std::uint32_t mask = 0xFFFFFFFFu);

} // namespace sc
