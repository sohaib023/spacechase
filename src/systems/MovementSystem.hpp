#pragma once

// Movement system: the momentum/drag model (migrated verbatim from M1's
// ship_step). Runs once per fixed timestep.

#include "ecs/Registry.hpp"

namespace sc {

// Copy each entity's Transform into its PrevTransform. Call at the start of
// every fixed step, before movement_step (enables render interpolation).
void snapshot_prev_transforms(Registry& registry);

// Integrate one fixed step for all ships (Transform, Velocity, ShipStats, ShipControl).
void movement_step(Registry& registry, float dt);

} // namespace sc
