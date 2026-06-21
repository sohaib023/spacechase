#include <doctest/doctest.h>

#include "components/ShipControl.hpp"
#include "components/ShipStats.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "ecs/Registry.hpp"
#include "systems/MovementSystem.hpp"

#include <glm/glm.hpp>

TEST_CASE("movement_step integrates thrust with drag") {
    sc::Registry reg;
    const sc::Entity e = reg.create();
    reg.emplace<sc::Transform>(e);
    reg.emplace<sc::Velocity>(e);
    reg.emplace<sc::ShipStats>(e, sc::ShipStats{900.0f, 600.0f, 3.2f, 1.2f});
    reg.emplace<sc::ShipControl>(e, sc::ShipControl{1.0f, 0.0f}); // full thrust, no turn

    constexpr float dt = 1.0f / 60.0f;
    sc::movement_step(reg, dt);

    // facing 0 -> forward = +Y. v = accel*dt, then drag scales by (1 - drag*dt).
    const float vExpected = (900.0f * dt) * (1.0f - 1.2f * dt);
    const sc::Velocity& vel = reg.get<sc::Velocity>(e);
    const sc::Transform& tr = reg.get<sc::Transform>(e);
    CHECK(vel.value.x == doctest::Approx(0.0f));
    CHECK(vel.value.y == doctest::Approx(vExpected));
    CHECK(tr.position.y == doctest::Approx(vExpected * dt));
}

TEST_CASE("movement_step clamps to max speed") {
    sc::Registry reg;
    const sc::Entity e = reg.create();
    reg.emplace<sc::Transform>(e);
    reg.emplace<sc::Velocity>(e, sc::Velocity{{0.0f, 1000.0f}}); // already above maxSpeed
    reg.emplace<sc::ShipStats>(e, sc::ShipStats{0.0f, 600.0f, 0.0f, 0.0f}); // no accel/drag
    reg.emplace<sc::ShipControl>(e);

    sc::movement_step(reg, 1.0f / 60.0f);

    const sc::Velocity& vel = reg.get<sc::Velocity>(e);
    CHECK(glm::length(vel.value) == doctest::Approx(600.0f));
}

TEST_CASE("movement_step turns the ship") {
    sc::Registry reg;
    const sc::Entity e = reg.create();
    reg.emplace<sc::Transform>(e);
    reg.emplace<sc::Velocity>(e);
    reg.emplace<sc::ShipStats>(e, sc::ShipStats{0.0f, 600.0f, 3.2f, 0.0f});
    reg.emplace<sc::ShipControl>(e, sc::ShipControl{0.0f, 1.0f}); // turn left (CCW)

    constexpr float dt = 1.0f / 60.0f;
    sc::movement_step(reg, dt);

    CHECK(reg.get<sc::Transform>(e).rotation == doctest::Approx(3.2f * dt));
}
