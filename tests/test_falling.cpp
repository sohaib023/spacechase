#include <doctest/doctest.h>

#include "components/Altitude.hpp"
#include "components/ShipControl.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "ecs/Registry.hpp"
#include "gameplay/FallConfig.hpp"
#include "gameplay/Level.hpp"
#include "systems/FloorAltitudeSystem.hpp"

#include <glm/glm.hpp>

using namespace sc;

namespace {
Entity make_ship(Registry& reg, glm::vec2 pos, float z) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{pos, 0.0f});
    reg.emplace<Velocity>(e);
    Altitude a;
    a.z = z;
    reg.emplace<Altitude>(e, a);
    return e;
}

Level floor_with_pit() {
    // Two floor rects with a gap at x in [4,6] (the void / pit).
    Level lvl;
    lvl.floorRects.push_back(AABB{{-3.0f, 0.0f}, {7.0f, 10.0f}}); // x in [-10, 4]
    lvl.floorRects.push_back(AABB{{8.0f, 0.0f}, {2.0f, 10.0f}});  // x in [6, 10]
    return lvl;
}
constexpr float kDt = 1.0f / 60.0f;
} // namespace

TEST_CASE("over floor: altitude holds at hover height") {
    Registry reg;
    const Level lvl = floor_with_pit();
    const FallConfig cfg;
    const Entity e = make_ship(reg, {0.0f, 0.0f}, cfg.hoverHeight);
    floor_altitude_step(reg, lvl, cfg, {0.0f, 0.0f}, kDt);
    CHECK(reg.get<Altitude>(e).z == doctest::Approx(cfg.hoverHeight));
    CHECK_FALSE(reg.get<Altitude>(e).falling);
}

TEST_CASE("over void: gravity lowers altitude and sets falling") {
    Registry reg;
    const Level lvl = floor_with_pit();
    const FallConfig cfg;
    const Entity e = make_ship(reg, {5.0f, 0.0f}, cfg.hoverHeight); // over the pit
    floor_altitude_step(reg, lvl, cfg, {0.0f, 0.0f}, kDt);
    const Altitude& a = reg.get<Altitude>(e);
    CHECK(a.falling);
    CHECK(a.z < cfg.hoverHeight);
    CHECK(a.vz < 0.0f);
}

TEST_CASE("returning over floor recovers (altitude rises)") {
    Registry reg;
    const Level lvl = floor_with_pit();
    const FallConfig cfg;
    const Entity e = make_ship(reg, {0.0f, 0.0f}, 0.5f); // over floor, below hover
    floor_altitude_step(reg, lvl, cfg, {0.0f, 0.0f}, kDt);
    const Altitude& a = reg.get<Altitude>(e);
    CHECK(a.z > 0.5f);
    CHECK(a.z <= cfg.hoverHeight);
}

TEST_CASE("fatal fall respawns at spawn") {
    Registry reg;
    const Level lvl = floor_with_pit();
    const FallConfig cfg;
    const Entity e = make_ship(reg, {5.0f, 0.0f}, cfg.deathZ + 0.001f); // over void, near death
    reg.get<Altitude>(e).vz = -10.0f;                                   // already plunging
    floor_altitude_step(reg, lvl, cfg, {0.0f, 0.0f}, kDt);
    CHECK(reg.get<Transform>(e).position.x == doctest::Approx(0.0f));
    CHECK(reg.get<Transform>(e).position.y == doctest::Approx(0.0f));
    CHECK(reg.get<Altitude>(e).z == doctest::Approx(cfg.hoverHeight));
    CHECK_FALSE(reg.get<Altitude>(e).falling);
}

TEST_CASE("control lock zeroes thrust only when falling and flag is off") {
    Registry reg;
    const Entity e = reg.create();
    reg.emplace<ShipControl>(e, ShipControl{1.0f, 1.0f});
    Altitude a;
    a.falling = true;
    reg.emplace<Altitude>(e, a);

    FallConfig cfg;
    cfg.controlWhileFalling = false;
    apply_fall_control(reg, cfg);
    CHECK(reg.get<ShipControl>(e).thrust == doctest::Approx(0.0f)); // thrust locked
    CHECK(reg.get<ShipControl>(e).turn == doctest::Approx(1.0f));   // rotation preserved

    reg.get<ShipControl>(e).thrust = 1.0f;
    cfg.controlWhileFalling = true;
    apply_fall_control(reg, cfg);
    CHECK(reg.get<ShipControl>(e).thrust == doctest::Approx(1.0f)); // flag on → no lock
}
