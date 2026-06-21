#include <doctest/doctest.h>

#include "components/CollisionLayer.hpp"
#include "components/Hitbox.hpp"
#include "components/StaticTag.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "ecs/Registry.hpp"
#include "systems/CollisionSystem.hpp"

#include <glm/glm.hpp>

using namespace sc;

namespace {
Entity make_wall(Registry& reg, glm::vec2 center, glm::vec2 half) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{center, 0.0f});
    Hitbox hb;
    hb.shape = HitboxShape::AABB;
    hb.halfExtents = half;
    reg.emplace<Hitbox>(e, hb);
    reg.emplace<CollisionLayer>(e, CollisionLayer{layer::Wall, 0u});
    reg.emplace<StaticTag>(e);
    return e;
}

Entity make_ship(Registry& reg, glm::vec2 pos, glm::vec2 vel, float radius) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{pos, 0.0f});
    reg.emplace<Velocity>(e, Velocity{vel});
    Hitbox hb;
    hb.shape = HitboxShape::Circle;
    hb.radius = radius;
    reg.emplace<Hitbox>(e, hb);
    reg.emplace<CollisionLayer>(e, CollisionLayer{layer::Ship, layer::Wall});
    return e;
}
} // namespace

TEST_CASE("collision pushes a ship out of a wall and cancels inward velocity") {
    Registry reg;
    make_wall(reg, {0.0f, 0.0f}, {1.0f, 1.0f}); // box x,y in [-1,1]
    // Ship overlapping the right face, moving left (into the wall).
    const Entity ship = make_ship(reg, {1.5f, 0.0f}, {-5.0f, 0.0f}, 1.0f);

    collision_step(reg, 1.0f / 60.0f);

    const Transform& t = reg.get<Transform>(ship);
    const Velocity& v = reg.get<Velocity>(ship);
    // Pushed out to just touching: center.x >= 2 - slop.
    CHECK(t.position.x >= doctest::Approx(2.0f).epsilon(0.01));
    // Inward (−x) velocity cancelled.
    CHECK(v.value.x == doctest::Approx(0.0f).epsilon(0.01));
}

TEST_CASE("glancing motion slides (tangential velocity preserved)") {
    Registry reg;
    make_wall(reg, {0.0f, 0.0f}, {1.0f, 1.0f});
    // Overlapping the right face but moving along +y (parallel to the face).
    const Entity ship = make_ship(reg, {1.5f, 0.0f}, {0.0f, 5.0f}, 1.0f);

    collision_step(reg, 1.0f / 60.0f);

    const Velocity& v = reg.get<Velocity>(ship);
    CHECK(v.value.y == doctest::Approx(5.0f)); // tangential component kept
}

TEST_CASE("no collision when separated") {
    Registry reg;
    make_wall(reg, {0.0f, 0.0f}, {1.0f, 1.0f});
    const Entity ship = make_ship(reg, {10.0f, 0.0f}, {-5.0f, 0.0f}, 1.0f);

    collision_step(reg, 1.0f / 60.0f);

    CHECK(reg.get<Transform>(ship).position.x == doctest::Approx(10.0f));
    CHECK(reg.get<Velocity>(ship).value.x == doctest::Approx(-5.0f));
}
