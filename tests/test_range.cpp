#include <doctest/doctest.h>

#include "components/Bounce.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Damage.hpp"
#include "components/Hitbox.hpp"
#include "components/Owner.hpp"
#include "components/PrevTransform.hpp"
#include "components/Projectile.hpp"
#include "components/Range.hpp"
#include "components/SurfaceResponse.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "ecs/Registry.hpp"
#include "gameplay/Spawning.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/ProjectileHitSystem.hpp"
#include "systems/ProjectileMoveSystem.hpp"
#include "systems/RangeSystem.hpp"

using namespace sc;

namespace {
constexpr float kDt = 1.0f / 60.0f;
}

TEST_CASE("range decrements by distance travelled and expires") {
    Registry reg;
    Deferred deferred;
    const Entity e = reg.create();
    reg.emplace<Velocity>(e, Velocity{{0.0f, 30.0f}}); // 0.5 m per step
    reg.emplace<Range>(e, Range{1.9f});

    range_step(reg, deferred, kDt);
    CHECK(reg.get<Range>(e).remaining == doctest::Approx(1.4f));
    CHECK(deferred.despawns.empty());

    range_step(reg, deferred, kDt); // 0.9
    range_step(reg, deferred, kDt); // 0.4
    CHECK(deferred.despawns.empty());
    range_step(reg, deferred, kDt); // -0.1 -> expired
    REQUIRE(deferred.despawns.size() == 1);
    CHECK(deferred.despawns[0] == e);
}

TEST_CASE("range is direction independent") {
    Registry reg;
    Deferred deferred;
    const Entity a = reg.create();
    reg.emplace<Velocity>(a, Velocity{{30.0f, 0.0f}});
    reg.emplace<Range>(a, Range{10.0f});
    const Entity b = reg.create();
    reg.emplace<Velocity>(b, Velocity{{-18.0f, 24.0f}}); // also 30 m/s
    reg.emplace<Range>(b, Range{10.0f});

    range_step(reg, deferred, kDt);
    CHECK(reg.get<Range>(a).remaining == doctest::Approx(reg.get<Range>(b).remaining));
}

TEST_CASE("a ricocheting bullet expires after the same distance as a straight one") {
    Registry reg;
    Deferred deferred;

    // A long reflective wall at y = 3 (face y = 2.5); bullet A bounces off it,
    // bullet B flies past it in open space. Same speed, same range.
    const Entity wall = reg.create();
    reg.emplace<Transform>(wall, Transform{{0.0f, 3.0f}, 0.0f});
    Hitbox hitbox;
    hitbox.shape = HitboxShape::AABB;
    hitbox.halfExtents = {5.0f, 0.5f};
    reg.emplace<Hitbox>(wall, hitbox);
    reg.emplace<CollisionLayer>(wall, CollisionLayer{layer::Wall, 0u});
    reg.emplace<SurfaceResponse>(wall, SurfaceResponse{SurfaceType::Reflective});

    const auto bullet = [&](glm::vec2 pos) {
        const Entity e = reg.create();
        reg.emplace<Projectile>(e);
        reg.emplace<Transform>(e, Transform{pos, 0.0f});
        reg.emplace<PrevTransform>(e, PrevTransform{pos, 0.0f});
        reg.emplace<Velocity>(e, Velocity{{0.0f, 30.0f}});
        reg.emplace<Owner>(e, Owner{entt::null, 0u});
        reg.emplace<Damage>(e, Damage{1.0f});
        reg.emplace<Range>(e, Range{10.0f});
        reg.emplace<Bounce>(e, Bounce{-1});
        return e;
    };
    const Entity bouncer = bullet({0.0f, 0.0f});
    const Entity straight = bullet({20.0f, 0.0f});

    int bouncerSteps = -1;
    int straightSteps = -1;
    bool bounced = false;
    for (int step = 1; step <= 120 && (bouncerSteps < 0 || straightSteps < 0); ++step) {
        snapshot_prev_transforms(reg);
        projectile_move_step(reg, kDt);
        projectile_hit_step(reg, deferred);
        range_step(reg, deferred, kDt);
        if (reg.valid(bouncer) && reg.get<Velocity>(bouncer).value.y < 0.0f) {
            bounced = true;
        }
        commit_deferred(reg, deferred);
        if (bouncerSteps < 0 && !reg.valid(bouncer)) bouncerSteps = step;
        if (straightSteps < 0 && !reg.valid(straight)) straightSteps = step;
    }

    CHECK(bounced);
    CHECK(bouncerSteps > 0);
    CHECK(bouncerSteps == straightSteps); // 10 m at 0.5 m/step -> 20 steps for both
}
