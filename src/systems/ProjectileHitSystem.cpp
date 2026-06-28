#include "systems/ProjectileHitSystem.hpp"

#include "components/CollisionLayer.hpp"
#include "components/Damage.hpp"
#include "components/Health.hpp"
#include "components/Owner.hpp"
#include "components/PrevTransform.hpp"
#include "components/Projectile.hpp"
#include "components/Transform.hpp"
#include "gameplay/Spawning.hpp"
#include "physics/Raycast.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace sc {

void projectile_hit_step(Registry& registry, Deferred& deferred) {
    auto view = registry.view<Projectile, PrevTransform, Transform, Owner, Damage>();
    // Stable id order keeps multi-projectile resolution deterministic.
    std::vector<Entity> ordered(view.begin(), view.end());
    std::sort(ordered.begin(), ordered.end());

    for (const Entity projectile : ordered) {
        const glm::vec2 from = view.get<PrevTransform>(projectile).position;
        const glm::vec2 to = view.get<Transform>(projectile).position;
        const glm::vec2 segment = to - from;
        const float dist = std::sqrt(glm::dot(segment, segment));
        if (dist < 1e-6f) {
            continue; // no movement this step -> nothing to sweep
        }

        const RayHit hit = raycast(registry, from, segment, dist, layer::Wall);
        if (!hit.hit || hit.entity == view.get<Owner>(projectile).entity) {
            continue;
        }

        if (Health* health = registry.try_get<Health>(hit.entity)) {
            health->current -= view.get<Damage>(projectile).amount;
            if (health->current <= 0.0f) {
                deferred.despawns.push_back(hit.entity); // destroys crates
            }
        }
        deferred.despawns.push_back(projectile);
    }
}

} // namespace sc
