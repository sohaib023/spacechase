#include "systems/RangeSystem.hpp"

#include "components/Range.hpp"
#include "components/Velocity.hpp"
#include "gameplay/Spawning.hpp"

namespace sc {

void range_step(Registry& registry, Deferred& deferred, float dt) {
    auto view = registry.view<Range, Velocity>();
    for (const Entity entity : view) {
        Range& range = view.get<Range>(entity);
        range.remaining -= glm::length(view.get<Velocity>(entity).value) * dt;
        if (range.remaining <= 0.0f) {
            deferred.despawns.push_back(entity);
        }
    }
}

} // namespace sc
