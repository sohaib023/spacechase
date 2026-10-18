#include "systems/ProjectileHitSystem.hpp"

#include "components/Bounce.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Damage.hpp"
#include "components/Health.hpp"
#include "components/Owner.hpp"
#include "components/PrevTransform.hpp"
#include "components/Projectile.hpp"
#include "components/SurfaceResponse.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "gameplay/Spawning.hpp"
#include "physics/Raycast.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace sc {

namespace {
constexpr int kMaxHitsPerStep = 8;   // ricochets resolved within one step (corner guard)
constexpr float kSurfaceEps = 1e-3f; // re-launch offset off the surface after a bounce
} // namespace

void projectile_hit_step(Registry& registry, Deferred& deferred) {
    auto view = registry.view<Projectile, PrevTransform, Transform, Velocity, Owner, Damage>();
    // Stable id order keeps multi-projectile resolution deterministic.
    std::vector<Entity> ordered(view.begin(), view.end());
    std::sort(ordered.begin(), ordered.end());

    for (const Entity projectile : ordered) {
        const glm::vec2 start = view.get<PrevTransform>(projectile).position;
        const glm::vec2 segment = view.get<Transform>(projectile).position - start;
        float remaining = std::sqrt(glm::dot(segment, segment));
        if (remaining < 1e-6f) {
            continue; // no movement this step -> nothing to sweep
        }

        const Entity owner = view.get<Owner>(projectile).entity;
        const float damage = view.get<Damage>(projectile).amount;
        Bounce* bounce = registry.try_get<Bounce>(projectile);

        // Walk the step's path segment by segment: each hit deals damage, then the
        // projectile either ricochets (bouncy + reflective surface + bounces left)
        // or is absorbed.
        glm::vec2 from = start;
        glm::vec2 dir = segment / remaining;
        bool absorbed = false;
        bool bounced = false;
        for (int i = 0; i < kMaxHitsPerStep && remaining > 1e-6f; ++i) {
            const RayHit hit = raycast(registry, from, dir, remaining, layer::Wall);
            if (!hit.hit || hit.entity == owner) {
                break;
            }

            if (Health* health = registry.try_get<Health>(hit.entity)) {
                health->current -= damage;
                if (health->current <= 0.0f) {
                    deferred.despawns.push_back(hit.entity); // destroys crates
                }
            }

            const SurfaceResponse* surface = registry.try_get<SurfaceResponse>(hit.entity);
            const bool reflective =
                surface != nullptr && surface->type == SurfaceType::Reflective;
            // A zero normal means the ray started inside the box: treat as absorbed.
            const bool validNormal = glm::dot(hit.normal, hit.normal) > 0.5f;
            if (bounce == nullptr || bounce->remaining == 0 || !reflective || !validNormal) {
                absorbed = true;
                break;
            }

            if (bounce->remaining > 0) {
                --bounce->remaining;
            }
            dir -= 2.0f * glm::dot(dir, hit.normal) * hit.normal;
            from = hit.point + hit.normal * kSurfaceEps;
            remaining -= hit.t;
            bounced = true;
        }

        if (absorbed) {
            deferred.despawns.push_back(projectile);
            continue;
        }
        if (bounced) {
            // Same speed, new heading; end the step where the path ends.
            Velocity& velocity = view.get<Velocity>(projectile);
            velocity.value = dir * glm::length(velocity.value);
            Transform& transform = view.get<Transform>(projectile);
            transform.position = from + dir * remaining;
            transform.rotation = std::atan2(-dir.x, dir.y); // facing 0 = +Y
        }
    }
}

} // namespace sc
