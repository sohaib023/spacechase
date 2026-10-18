#include <doctest/doctest.h>

#include "components/Bounce.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Damage.hpp"
#include "components/Health.hpp"
#include "components/Hitbox.hpp"
#include "components/Owner.hpp"
#include "components/PrevTransform.hpp"
#include "components/Projectile.hpp"
#include "components/SurfaceResponse.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
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
    reg.emplace<Velocity>(e, Velocity{(to - from) * 60.0f}); // swept over one 1/60 s step
    reg.emplace<Owner>(e, Owner{owner, 0u});
    reg.emplace<Damage>(e, Damage{damage});
    return e;
}

void make_reflective(Registry& reg, Entity e) {
    reg.emplace<SurfaceResponse>(e, SurfaceResponse{SurfaceType::Reflective});
}

bool queued(const Deferred& deferred, Entity e) {
    for (const Entity d : deferred.despawns) {
        if (d == e) {
            return true;
        }
    }
    return false;
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

// --- Ricochet ------------------------------------------------------------------

TEST_CASE("a bouncy projectile ricochets off a reflective wall and keeps its speed") {
    Registry reg;
    Deferred deferred;
    const Entity wall = make_crate(reg, {0.0f, 0.0f}, {0.5f, 0.5f}, 100.0f);
    make_reflective(reg, wall);
    // Path (0,-2) -> (0,2): hits the face y = -0.5 after 1.5 m, 2.5 m left to travel.
    const Entity proj = make_projectile(reg, {0.0f, -2.0f}, {0.0f, 2.0f}, 25.0f, entt::null);
    reg.emplace<Bounce>(proj, Bounce{-1});
    const float speedBefore = glm::length(reg.get<Velocity>(proj).value);

    projectile_hit_step(reg, deferred);

    CHECK_FALSE(queued(deferred, proj));
    CHECK(reg.get<Health>(wall).current == doctest::Approx(75.0f)); // damage on the bounce hit
    const glm::vec2 v = reg.get<Velocity>(proj).value;
    CHECK(v.y < 0.0f);                                              // mirrored
    CHECK(glm::length(v) == doctest::Approx(speedBefore));
    CHECK(reg.get<Transform>(proj).position.y == doctest::Approx(-3.0f).epsilon(0.01));
    CHECK(reg.get<Bounce>(proj).remaining == -1);                   // unlimited stays unlimited
}

TEST_CASE("an absorbent surface stops even a bouncy projectile") {
    Registry reg;
    Deferred deferred;
    make_crate(reg, {0.0f, 0.0f}, {0.5f, 0.5f}, 100.0f); // no SurfaceResponse -> absorbent
    const Entity proj = make_projectile(reg, {0.0f, -2.0f}, {0.0f, 2.0f}, 25.0f, entt::null);
    reg.emplace<Bounce>(proj, Bounce{-1});

    projectile_hit_step(reg, deferred);

    CHECK(queued(deferred, proj));
}

TEST_CASE("a non-bouncy projectile is absorbed by a reflective wall") {
    Registry reg;
    Deferred deferred;
    const Entity wall = make_crate(reg, {0.0f, 0.0f}, {0.5f, 0.5f}, 100.0f);
    make_reflective(reg, wall);
    const Entity proj = make_projectile(reg, {0.0f, -2.0f}, {0.0f, 2.0f}, 25.0f, entt::null);

    projectile_hit_step(reg, deferred);

    CHECK(queued(deferred, proj));
}

TEST_CASE("the bounce limit is honored") {
    Registry reg;
    Deferred deferred;
    const Entity wall = make_crate(reg, {0.0f, 0.0f}, {0.5f, 0.5f}, 100.0f);
    make_reflective(reg, wall);
    const Entity once = make_projectile(reg, {0.0f, -2.0f}, {0.0f, 2.0f}, 1.0f, entt::null);
    reg.emplace<Bounce>(once, Bounce{1});
    const Entity spent = make_projectile(reg, {0.2f, -2.0f}, {0.2f, 2.0f}, 1.0f, entt::null);
    reg.emplace<Bounce>(spent, Bounce{0});

    projectile_hit_step(reg, deferred);

    CHECK_FALSE(queued(deferred, once));
    CHECK(reg.get<Bounce>(once).remaining == 0);
    CHECK(queued(deferred, spent));
}

TEST_CASE("every bounce hit deals damage again") {
    Registry reg;
    Deferred deferred;
    // A corridor: reflective walls above (faces y = 0.5) and below (faces y = -0.5).
    const Entity top = make_crate(reg, {0.0f, 1.0f}, {5.0f, 0.5f}, 100.0f);
    const Entity bottom = make_crate(reg, {0.0f, -1.0f}, {5.0f, 0.5f}, 100.0f);
    make_reflective(reg, top);
    make_reflective(reg, bottom);
    // 3 m of travel this step: up 0.5 (hit top), down 1.0 (hit bottom), up 1.0 (hit top), 0.5 left.
    const Entity proj = make_projectile(reg, {0.0f, 0.0f}, {0.0f, 3.0f}, 10.0f, entt::null);
    reg.emplace<Bounce>(proj, Bounce{-1});

    projectile_hit_step(reg, deferred);

    CHECK_FALSE(queued(deferred, proj));
    CHECK(reg.get<Health>(top).current == doctest::Approx(80.0f));    // hit twice
    CHECK(reg.get<Health>(bottom).current == doctest::Approx(90.0f)); // hit once
}
