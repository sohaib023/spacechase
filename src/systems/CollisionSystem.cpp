#include "systems/CollisionSystem.hpp"

#include "components/CollisionLayer.hpp"
#include "components/Hitbox.hpp"
#include "components/StaticTag.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "physics/Overlap.hpp"
#include "physics/Shapes.hpp"
#include "physics/SpatialHash.hpp"

#include <algorithm>
#include <vector>

namespace sc {

namespace {

constexpr float kCellSize = 4.0f;  // meters; low entity counts in M3
constexpr float kSlop = 0.001f;    // skin width to avoid jitter
constexpr int kMaxIterations = 3;  // corner/seam convergence cap

AABB world_bounds(const Transform& transform, const Hitbox& hitbox) {
    const glm::vec2 center = transform.position + hitbox.offset;
    if (hitbox.shape == HitboxShape::Circle) {
        return AABB{center, glm::vec2(hitbox.radius)};
    }
    return AABB{center, hitbox.halfExtents};
}

} // namespace

void collision_step(Registry& registry, float dt) {
    // Rebuild the broadphase from every collider this step (statics included;
    // few entities in M3, so a full rebuild is fine).
    SpatialHash hash(kCellSize);
    auto colliders = registry.view<Transform, Hitbox, CollisionLayer>();
    for (const Entity entity : colliders) {
        hash.insert(entity, world_bounds(registry.get<Transform>(entity),
                                         registry.get<Hitbox>(entity)));
    }

    // Resolve dynamic circle movers (entities with Velocity), in stable id order.
    auto movers = registry.view<Transform, Velocity, Hitbox, CollisionLayer>();
    std::vector<Entity> ordered(movers.begin(), movers.end());
    std::sort(ordered.begin(), ordered.end());

    std::vector<Entity> candidates;
    for (const Entity self : ordered) {
        if (registry.all_of<StaticTag>(self)) {
            continue;
        }
        const Hitbox& selfHitbox = registry.get<Hitbox>(self);
        if (selfHitbox.shape != HitboxShape::Circle) {
            continue; // M3 only resolves circle movers
        }
        const std::uint32_t selfMask = registry.get<CollisionLayer>(self).mask;

        for (int iteration = 0; iteration < kMaxIterations; ++iteration) {
            Transform& selfTransform = registry.get<Transform>(self);
            Velocity& selfVelocity = registry.get<Velocity>(self);
            const Circle circle{selfTransform.position + selfHitbox.offset, selfHitbox.radius};

            // Swept broadphase: expand the query by this step's displacement so a
            // fast mover still finds the wall it is about to touch.
            const glm::vec2 displacement = selfVelocity.value * dt;
            const AABB query{circle.center, glm::vec2(selfHitbox.radius) + glm::abs(displacement)};
            candidates.clear();
            hash.query(query, candidates);

            // Deepest contact wins this iteration (stable: candidates are id-sorted).
            Manifold deepest;
            for (const Entity other : candidates) {
                if (other == self) {
                    continue;
                }
                if ((selfMask & registry.get<CollisionLayer>(other).layer) == 0) {
                    continue;
                }
                const Transform& otherTransform = registry.get<Transform>(other);
                const Hitbox& otherHitbox = registry.get<Hitbox>(other);
                const glm::vec2 otherCenter = otherTransform.position + otherHitbox.offset;
                const Manifold m =
                    otherHitbox.shape == HitboxShape::AABB
                        ? circle_vs_aabb(circle, AABB{otherCenter, otherHitbox.halfExtents})
                        : circle_vs_circle(circle, Circle{otherCenter, otherHitbox.radius});
                if (m.hit && m.penetration > deepest.penetration) {
                    deepest = m;
                }
            }

            if (!deepest.hit) {
                break;
            }
            // Push out of the deepest penetration and cancel inward velocity
            // (stop head-on, slide when glancing — no bounce).
            if (deepest.penetration > kSlop) {
                selfTransform.position += deepest.normal * (deepest.penetration - kSlop);
            }
            const float normalSpeed = glm::dot(selfVelocity.value, deepest.normal);
            if (normalSpeed < 0.0f) {
                selfVelocity.value -= normalSpeed * deepest.normal;
            }
        }
    }
}

} // namespace sc
