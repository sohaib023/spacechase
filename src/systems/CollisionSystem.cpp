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
#include <cmath>
#include <vector>

namespace sc {

namespace {

constexpr float kCellSize = 4.0f;  // meters; low entity counts for now
constexpr float kSlop = 0.001f;    // skin width to avoid jitter
constexpr int kMaxIterations = 3;  // corner/seam convergence cap

// Build the world-space convex polygon for an AABB or Polygon hitbox (rotated by
// the entity's rotation; for an AABB this yields an oriented box when rotated).
Convex to_convex(const Hitbox& hitbox, glm::vec2 center, float rotation) {
    Convex convex;
    const float cosR = std::cos(rotation);
    const float sinR = std::sin(rotation);
    if (hitbox.shape == HitboxShape::AABB) {
        const glm::vec2 h = hitbox.halfExtents;
        const glm::vec2 local[4] = {{-h.x, -h.y}, {h.x, -h.y}, {h.x, h.y}, {-h.x, h.y}};
        convex.count = 4;
        for (int i = 0; i < 4; ++i) {
            convex.verts[i] = rotate_point(local[i], cosR, sinR) + center;
        }
    } else { // Polygon
        convex.count = std::min(hitbox.vertexCount, kMaxHitboxVerts);
        for (int i = 0; i < convex.count; ++i) {
            convex.verts[i] = rotate_point(hitbox.vertices[i], cosR, sinR) + center;
        }
    }
    return convex;
}

AABB world_bounds(const Transform& transform, const Hitbox& hitbox) {
    const glm::vec2 center = transform.position + hitbox.offset;
    if (hitbox.shape == HitboxShape::Circle) {
        return AABB{center, glm::vec2(hitbox.radius)};
    }
    const Convex convex = to_convex(hitbox, center, transform.rotation);
    glm::vec2 lo = convex.verts[0];
    glm::vec2 hi = convex.verts[0];
    for (int i = 1; i < convex.count; ++i) {
        lo = glm::min(lo, convex.verts[i]);
        hi = glm::max(hi, convex.verts[i]);
    }
    return AABB{(lo + hi) * 0.5f, (hi - lo) * 0.5f};
}

// Collision between any two hitboxes. The returned manifold normal pushes A out of B.
Manifold collide(const Hitbox& a, glm::vec2 aCenter, float aRot, const Hitbox& b,
                 glm::vec2 bCenter, float bRot) {
    const bool aCircle = a.shape == HitboxShape::Circle;
    const bool bCircle = b.shape == HitboxShape::Circle;
    if (aCircle && bCircle) {
        return circle_vs_circle(Circle{aCenter, a.radius}, Circle{bCenter, b.radius});
    }
    if (aCircle) {
        return circle_vs_convex(Circle{aCenter, a.radius}, to_convex(b, bCenter, bRot));
    }
    if (bCircle) {
        Manifold m = circle_vs_convex(Circle{bCenter, b.radius}, to_convex(a, aCenter, aRot));
        m.normal = -m.normal; // circle_vs_convex pushes B out; flip to push A out
        return m;
    }
    return convex_vs_convex(to_convex(a, aCenter, aRot), to_convex(b, bCenter, bRot));
}

} // namespace

void collision_step(Registry& registry, float dt) {
    // Rebuild the broadphase from every collider this step.
    SpatialHash hash(kCellSize);
    auto colliders = registry.view<Transform, Hitbox, CollisionLayer>();
    for (const Entity entity : colliders) {
        hash.insert(entity, world_bounds(registry.get<Transform>(entity),
                                         registry.get<Hitbox>(entity)));
    }

    // Resolve dynamic movers (entities with Velocity), in stable id order.
    auto movers = registry.view<Transform, Velocity, Hitbox, CollisionLayer>();
    std::vector<Entity> ordered(movers.begin(), movers.end());
    std::sort(ordered.begin(), ordered.end());

    std::vector<Entity> candidates;
    for (const Entity self : ordered) {
        if (registry.all_of<StaticTag>(self)) {
            continue;
        }
        const Hitbox& selfHitbox = registry.get<Hitbox>(self);
        const std::uint32_t selfMask = registry.get<CollisionLayer>(self).mask;

        for (int iteration = 0; iteration < kMaxIterations; ++iteration) {
            Transform& selfTransform = registry.get<Transform>(self);
            Velocity& selfVelocity = registry.get<Velocity>(self);
            const glm::vec2 selfCenter = selfTransform.position + selfHitbox.offset;
            const float selfRot = selfTransform.rotation;

            // Swept broadphase: expand the mover's bounds by this step's displacement.
            AABB bounds = world_bounds(selfTransform, selfHitbox);
            bounds.halfExtents += glm::abs(selfVelocity.value * dt);
            candidates.clear();
            hash.query(bounds, candidates);

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
                const Manifold m = collide(selfHitbox, selfCenter, selfRot, otherHitbox,
                                           otherCenter, otherTransform.rotation);
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
