#include <doctest/doctest.h>

#include "components/BeamRequest.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Health.hpp"
#include "components/Hitbox.hpp"
#include "components/Transform.hpp"
#include "ecs/Registry.hpp"
#include "gameplay/Spawning.hpp"
#include "systems/BeamSystem.hpp"

using namespace sc;

namespace {
constexpr float kDt = 0.1f;

Entity make_crate(Registry& reg, glm::vec2 pos, float health) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{pos, 0.0f});
    Hitbox hitbox;
    hitbox.shape = HitboxShape::AABB;
    hitbox.halfExtents = {0.5f, 0.5f};
    reg.emplace<Hitbox>(e, hitbox);
    reg.emplace<CollisionLayer>(e, CollisionLayer{layer::Wall, 0u});
    reg.emplace<Health>(e, Health{health, health});
    return e;
}

Entity make_beamer(Registry& reg, bool active, float damage, float maxDist) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{{0.0f, 0.0f}, 0.0f}); // facing +Y
    BeamRequest beam;
    beam.active = active;
    beam.damage = damage;
    beam.maxDist = maxDist;
    reg.emplace<BeamRequest>(e, beam);
    return e;
}
} // namespace

TEST_CASE("an active beam damages the crate it hits and clips at its face") {
    Registry reg;
    Deferred deferred;
    BeamVizList viz;
    const Entity crate = make_crate(reg, {0.0f, 5.0f}, 100.0f);
    make_beamer(reg, /*active=*/true, /*damage=*/40.0f, /*maxDist=*/30.0f);

    beam_step(reg, viz, deferred, kDt);

    CHECK(reg.get<Health>(crate).current == doctest::Approx(96.0f)); // 40/s * 0.1
    REQUIRE(viz.beams.size() == 1);
    CHECK(viz.beams[0].end.y < 5.0f);   // clipped at the crate's near face
    CHECK(viz.beams[0].end.y > 4.0f);
}

TEST_CASE("an inactive beam does nothing and records no viz") {
    Registry reg;
    Deferred deferred;
    BeamVizList viz;
    const Entity crate = make_crate(reg, {0.0f, 5.0f}, 100.0f);
    make_beamer(reg, /*active=*/false, 40.0f, 30.0f);

    beam_step(reg, viz, deferred, kDt);

    CHECK(reg.get<Health>(crate).current == doctest::Approx(100.0f));
    CHECK(viz.beams.empty());
}

TEST_CASE("a beam with no wall in range draws to its full length, deals no damage") {
    Registry reg;
    Deferred deferred;
    BeamVizList viz;
    make_beamer(reg, /*active=*/true, 40.0f, /*maxDist=*/8.0f);

    beam_step(reg, viz, deferred, kDt);

    REQUIRE(viz.beams.size() == 1);
    // origin is 1 m ahead of the hull; full beam reaches origin + maxDist along +Y.
    CHECK(viz.beams[0].end.y == doctest::Approx(9.0f));
}

TEST_CASE("a lethal beam tick queues the crate for despawn") {
    Registry reg;
    Deferred deferred;
    BeamVizList viz;
    const Entity crate = make_crate(reg, {0.0f, 5.0f}, 2.0f);
    make_beamer(reg, /*active=*/true, /*damage=*/40.0f, 30.0f); // 4 dmg this tick

    beam_step(reg, viz, deferred, kDt);

    CHECK(reg.get<Health>(crate).current <= 0.0f);
    REQUIRE(deferred.despawns.size() == 1);
    CHECK(deferred.despawns[0] == crate);
}
