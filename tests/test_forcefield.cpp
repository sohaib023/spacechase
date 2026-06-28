#include <doctest/doctest.h>

#include "components/Deflector.hpp"
#include "components/ForceField.hpp"
#include "components/ForceResponse.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "ecs/Registry.hpp"
#include "systems/ForceFieldSystem.hpp"

#include <glm/glm.hpp>

using namespace sc;

namespace {
Entity deflectable(Registry& reg, glm::vec2 pos) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{pos, 0.0f});
    reg.emplace<Velocity>(e);
    reg.emplace<ForceResponse>(e, ForceResponse{ForceResponseType::Deflectable});
    return e;
}
Entity radial_source(Registry& reg, glm::vec2 pos, float strength, float range) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{pos, 0.0f});
    ForceField f;
    f.shape = FieldShape::Radial;
    f.strength = strength;
    f.range = range;
    f.falloff = FalloffType::None;
    reg.emplace<ForceField>(e, f);
    return e;
}
} // namespace

TEST_CASE("radial field pushes a deflectable target outward") {
    Registry reg;
    radial_source(reg, {0.0f, 0.0f}, 10.0f, 10.0f);
    const Entity t = deflectable(reg, {2.0f, 0.0f});
    forcefield_step(reg, 0.1f);
    CHECK(reg.get<Velocity>(t).value.x == doctest::Approx(1.0f)); // 10 * 0.1, +x outward
}

TEST_CASE("rigid targets are never pushed") {
    Registry reg;
    radial_source(reg, {0.0f, 0.0f}, 10.0f, 10.0f);
    const Entity t = reg.create();
    reg.emplace<Transform>(t, Transform{{2.0f, 0.0f}, 0.0f});
    reg.emplace<Velocity>(t);
    reg.emplace<ForceResponse>(t, ForceResponse{ForceResponseType::Rigid});
    forcefield_step(reg, 0.1f);
    CHECK(reg.get<Velocity>(t).value.x == doctest::Approx(0.0f));
}

TEST_CASE("directional wind adds velocity inside its range only") {
    Registry reg;
    const Entity src = reg.create();
    reg.emplace<Transform>(src, Transform{{0.0f, 0.0f}, 0.0f});
    ForceField f;
    f.shape = FieldShape::Directional;
    f.dir = {1.0f, 0.0f};
    f.strength = 5.0f;
    f.range = 4.0f;
    f.falloff = FalloffType::None;
    reg.emplace<ForceField>(src, f);
    const Entity inside = deflectable(reg, {1.0f, 0.0f});
    const Entity outside = deflectable(reg, {100.0f, 0.0f});
    forcefield_step(reg, 0.1f);
    CHECK(reg.get<Velocity>(inside).value.x == doctest::Approx(0.5f)); // 5 * 0.1
    CHECK(reg.get<Velocity>(outside).value.x == doctest::Approx(0.0f));
}

TEST_CASE("active deflector repels; inactive does nothing; never self-pushes") {
    Registry reg;
    const Entity ship = reg.create();
    reg.emplace<Transform>(ship, Transform{{0.0f, 0.0f}, 0.0f});
    reg.emplace<Velocity>(ship);
    reg.emplace<ForceResponse>(ship, ForceResponse{ForceResponseType::Deflectable});
    Deflector d;
    d.active = false;
    d.strength = 20.0f;
    d.range = 10.0f;
    reg.emplace<Deflector>(ship, d);
    const Entity bullet = deflectable(reg, {2.0f, 0.0f});

    forcefield_step(reg, 0.1f);
    CHECK(reg.get<Velocity>(bullet).value.x == doctest::Approx(0.0f)); // inactive → nothing

    reg.get<Deflector>(ship).active = true;
    forcefield_step(reg, 0.1f);
    CHECK(reg.get<Velocity>(bullet).value.x > 0.0f);                 // repelled outward
    CHECK(reg.get<Velocity>(ship).value.x == doctest::Approx(0.0f)); // self skipped
}
