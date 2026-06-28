#include "systems/LifetimeSystem.hpp"

#include "components/Lifetime.hpp"
#include "gameplay/Spawning.hpp"

namespace sc {

void lifetime_step(Registry& registry, Deferred& deferred, float dt) {
    auto view = registry.view<Lifetime>();
    for (const Entity entity : view) {
        Lifetime& lifetime = view.get<Lifetime>(entity);
        lifetime.remaining -= dt;
        if (lifetime.remaining <= 0.0f) {
            deferred.despawns.push_back(entity);
        }
    }
}

} // namespace sc
