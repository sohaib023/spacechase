#pragma once

#include "ecs/Registry.hpp"

namespace sc {

struct Deferred;

// Decrement every Lifetime by dt; queue entities whose lifetime has elapsed for
// despawn (applied in commit_deferred).
void lifetime_step(Registry& registry, Deferred& deferred, float dt);

} // namespace sc
