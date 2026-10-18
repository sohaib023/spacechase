#pragma once

#include "ecs/Registry.hpp"

namespace sc {

struct Deferred;

// Decrement every Range by the distance covered this step (|velocity| * dt) and
// queue entities that have used up their range for despawn. Bouncing never
// changes speed, so this is exact for ricochets too; the bounce code never
// touches Range. Despawn lands at the end of the step in which range runs out.
void range_step(Registry& registry, Deferred& deferred, float dt);

} // namespace sc
