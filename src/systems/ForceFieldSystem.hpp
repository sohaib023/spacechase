#pragma once

// Applies force-field sources (ForceField entities + active Deflector ships) to
// Deflectable targets each fixed step, via physics/ForceApi. Runs before
// collision so pushes resolve against walls the same step.

#include "ecs/Registry.hpp"

namespace sc {

void forcefield_step(Registry& registry, float dt);

} // namespace sc
