#pragma once

// Collision system: resolves dynamic (circle) colliders against everything they
// mask, after movement integration. Discrete "move then push out" — correct for
// the momentum/drag integrator (not a constraint solver). Runs in the fixed step.

#include "ecs/Registry.hpp"

namespace sc {

void collision_step(Registry& registry, float dt);

} // namespace sc
