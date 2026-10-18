#include <doctest/doctest.h>

#include "components/BeamRequest.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Health.hpp"
#include "components/Hitbox.hpp"
#include "components/SurfaceResponse.hpp"
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

// A wall of any size on the Wall layer, optionally reflective, with Health.
Entity make_wall(Registry& reg, glm::vec2 pos, glm::vec2 half, bool reflective) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{pos, 0.0f});
    Hitbox hitbox;
    hitbox.shape = HitboxShape::AABB;
    hitbox.halfExtents = half;
    reg.emplace<Hitbox>(e, hitbox);
    reg.emplace<CollisionLayer>(e, CollisionLayer{layer::Wall, 0u});
    reg.emplace<Health>(e, Health{100.0f, 100.0f});
    reg.emplace<SurfaceResponse>(
        e, SurfaceResponse{reflective ? SurfaceType::Reflective : SurfaceType::Absorbent});
    return e;
}

void make_bouncy(Registry& reg, Entity beamer, int maxBounces) {
    BeamRequest& beam = reg.get<BeamRequest>(beamer);
    beam.bounce = true;
    beam.maxBounces = maxBounces;
}

float total_length(const BeamVizList& viz) {
    float sum = 0.0f;
    for (const BeamSegment& s : viz.beams) {
        sum += glm::length(s.end - s.start);
    }
    return sum;
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

// --- Bouncing beams ------------------------------------------------------------

TEST_CASE("a bouncy beam reflects and its total length equals its range") {
    Registry reg;
    Deferred deferred;
    BeamVizList viz;
    make_wall(reg, {0.0f, 5.0f}, {0.5f, 0.5f}, /*reflective=*/true); // face y = 4.5
    const Entity beamer = make_beamer(reg, true, 40.0f, /*maxDist=*/10.0f);
    make_bouncy(reg, beamer, -1);

    beam_step(reg, viz, deferred, kDt);

    REQUIRE(viz.beams.size() == 2);
    CHECK(viz.beams[0].end.y == doctest::Approx(4.5f).epsilon(0.01));
    CHECK(viz.beams[1].end.y < viz.beams[1].start.y);               // heading back down
    CHECK(total_length(viz) == doctest::Approx(10.0f).epsilon(0.01)); // range shared
}

TEST_CASE("an absorbent surface stops a bouncy beam") {
    Registry reg;
    Deferred deferred;
    BeamVizList viz;
    make_wall(reg, {0.0f, 5.0f}, {0.5f, 0.5f}, /*reflective=*/false);
    const Entity beamer = make_beamer(reg, true, 40.0f, 30.0f);
    make_bouncy(reg, beamer, -1);

    beam_step(reg, viz, deferred, kDt);

    CHECK(viz.beams.size() == 1);
}

TEST_CASE("a non-bouncy beam stops at a reflective wall") {
    Registry reg;
    Deferred deferred;
    BeamVizList viz;
    make_wall(reg, {0.0f, 5.0f}, {0.5f, 0.5f}, /*reflective=*/true);
    make_beamer(reg, true, 40.0f, 30.0f);

    beam_step(reg, viz, deferred, kDt);

    CHECK(viz.beams.size() == 1);
}

TEST_CASE("beam bounce limit is honored and every hit takes damage") {
    Registry reg;
    Deferred deferred;
    BeamVizList viz;
    // Corridor: top face y = 2.5, bottom face y = -2.5; beam starts at (0, 1) going up.
    const Entity top = make_wall(reg, {0.0f, 3.0f}, {5.0f, 0.5f}, true);
    const Entity bottom = make_wall(reg, {0.0f, -3.0f}, {5.0f, 0.5f}, true);
    const Entity beamer = make_beamer(reg, true, /*damage=*/40.0f, /*maxDist=*/30.0f);
    make_bouncy(reg, beamer, 2);

    beam_step(reg, viz, deferred, kDt);

    // top (bounce 1) -> bottom (bounce 2) -> top (no bounces left, stop): 3 segments.
    CHECK(viz.beams.size() == 3);
    CHECK(reg.get<Health>(top).current == doctest::Approx(92.0f));    // hit twice, 4 each
    CHECK(reg.get<Health>(bottom).current == doctest::Approx(96.0f)); // hit once
}