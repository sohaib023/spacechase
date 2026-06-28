#pragma once

#include "ecs/Registry.hpp"

namespace sc {

// Straight-line integration for projectiles: Transform += Velocity * dt. Separate
// from movement_step because projectiles have no momentum/drag/turn (no ShipStats).
void projectile_move_step(Registry& registry, float dt);

} // namespace sc
