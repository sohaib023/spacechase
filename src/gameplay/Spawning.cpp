#include "gameplay/Spawning.hpp"

#include "gameplay/ProjectileFactory.hpp"

namespace sc {

void commit_deferred(Registry& registry, Deferred& deferred) {
    for (const ProjectileSpawn& spawn : deferred.spawns) {
        spawn_projectile(registry, spawn);
    }
    deferred.spawns.clear();

    // Despawns may contain duplicates (e.g. a projectile that both hit and
    // expired the same step) — valid() guards against double-destroy.
    for (const Entity entity : deferred.despawns) {
        if (registry.valid(entity)) {
            registry.destroy(entity);
        }
    }
    deferred.despawns.clear();
}

} // namespace sc
