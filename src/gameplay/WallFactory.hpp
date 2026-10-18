#pragma once

// Composes a static wall entity (AABB collider). M3.4 takes direct params and
// is called from an in-code test scene; a data-driven WallDef + JSON level
// loader replaces the in-code placement in M3.7.

#include "components/SurfaceResponse.hpp"
#include "ecs/Registry.hpp"
#include "resources/TextureHandle.hpp"

#include <glm/glm.hpp>

namespace sc {

Entity spawn_wall(Registry& registry, TextureHandle texture, TextureHandle normal,
                  glm::vec2 center, glm::vec2 halfExtents, bool destructible, float health,
                  SurfaceType surface);

} // namespace sc
