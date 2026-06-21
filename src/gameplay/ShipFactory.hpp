#pragma once

// Composes a ship entity from a definition. The data -> entity step that
// projectiles/abilities reuse in M5.

#include "ecs/Registry.hpp"
#include "resources/TextureHandle.hpp"

#include <glm/glm.hpp>

namespace sc {

struct ShipDef;

Entity spawn_ship(Registry& registry, const ShipDef& def, TextureHandle texture,
                  glm::vec2 position, float facing, bool playerControlled);

} // namespace sc
