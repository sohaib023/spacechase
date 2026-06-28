#pragma once

// Composes a projectile entity from a queued ProjectileSpawn (mirrors ShipFactory).
// PrevTransform starts equal to Transform so the first frame's interpolation is a
// no-op (no spawn streak). Drained from the spawn queue in commit_deferred.

#include "ecs/Registry.hpp"

namespace sc {

struct ProjectileSpawn;

Entity spawn_projectile(Registry& registry, const ProjectileSpawn& spawn);

} // namespace sc
