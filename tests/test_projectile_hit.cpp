#include <doctest/doctest.h>

#include "components/CollisionLayer.hpp"
#include "components/Damage.hpp"
#include "components/Health.hpp"
#include "components/Hitbox.hpp"
#include "components/Owner.hpp"
#include "components/PrevTransform.hpp"
#include "components/Projectile.hpp"
#include "components/Transform.hpp"
#include "ecs/Registry.hpp"
#include "gameplay/Spawning.hpp"
#include "systems/ProjectileHitSystem.hpp"

using namespace sc;

namespace {
// An AABB collider on the Wall layer (crate when given Health).
Entity make_crate(Registry& reg, glm::vec2 pos, glm::vec2 half, float health) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{pos, 0.0f});
    Hitbox hitbox;
    hitbox.shape = HitboxShape::AABB;
    hitbox.halfExtents = half;
    reg.emplace<Hitbox>(e, hitbox);
    reg.emplace<CollisionLayer>(e, CollisionLayer{layer::Wall, 0u});
    reg.emplace<Health>(e, Health{health, health});
    return e;
}

// A projectile whose swept segment runs from `from` to `to`.
Entity make_projectile(Registry& reg, glm::vec2 from, glm::vec2 to, float damage, Entity owner) {
    const Entity e = reg.create();
    reg.emplace<Projectile>(e);
    reg.emplace<PrevTransform>(e, PrevTransform{from, 0.0f});
    reg.emplace<Transform>(e, Transform{to, 0.0f});
    reg.emplace<Owner>(e, Owner{owner, 0u});
    reg.emplace<Damage>(e, Damage{damage});
    return e;
}
} // namespace

TEST_CASE("projectile damages a crate it sweeps through and despawns") {
    Registry reg;
    Deferred deferred;
    const Entity crate = make_crate(reg, {0.0f, 0.0f}, {0.5f, 0.5f}, 100.0f);
    const Entity proj = make_projectile(reg, {0.0f, -2.0f}, {0.0f, 2.0f}, 25.0f, entt::null);

    projectile_hit_step(reg, deferred);

    CHECK(reg.get<Health>(crate).current == doctest::Approx(75.0f));
    // crate survives (75 hp) so only the projectile is queued for despawn
    CHECK(deferred.despawns.size() == 1);
    CHECK(deferred.despawns[0] == proj);
}

TEST_CASE("a lethal hit queues both the crate and the projectile for despawn") {
    Registry reg;
    Deferred deferred;
    const Entity crate = make_crate(reg, {0.0f, 0.0f}, {0.5f, 0.5f}, 20.0f);
    make_projectile(reg, {0.0f, -2.0f}, {0.0f, 2.0f}, 25.0f, entt::null);

    projectile_hit_step(reg, deferred);

    CHECK(reg.get<Health>(crate).current <= 0.0f);
    CHECK(deferred.despawns.size() == 2);
    commit_deferred(reg, deferred);
    CHECK_FALSE(reg.valid(crate));
}

TEST_CASE("a projectile that misses leaves everything untouched") {
    Registry reg;
    Deferred deferred;
    const Entity crate = make_crate(reg, {10.0f, 10.0f}, {0.5f, 0.5f}, 100.0f);
    make_projectile(reg, {0.0f, -2.0f}, {0.0f, 2.0f}, 25.0f, entt::null);

    projectile_hit_step(reg, deferred);

    CHECK(reg.get<Health>(crate).current == doctest::Approx(100.0f));
    CHECK(deferred.despawns.empty());
}

TEST_CASE("a projectile never hits its own owner") {
    Registry reg;
    Deferred deferred;
    const Entity crate = make_crate(reg, {0.0f, 0.0f}, {0.5f, 0.5f}, 100.0f);
    // owner == the crate it sweeps through -> skipped.
    make_projectile(reg, {0.0f, -2.0f}, {0.0f, 2.0f}, 25.0f, crate);

    projectile_hit_step(reg, deferred);

    CHECK(reg.get<Health>(crate).current == doctest::Approx(100.0f));
    CHECK(deferred.despawns.empty());
}
