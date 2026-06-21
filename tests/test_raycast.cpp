#include <doctest/doctest.h>

#include "components/CollisionLayer.hpp"
#include "components/Hitbox.hpp"
#include "components/Transform.hpp"
#include "ecs/Registry.hpp"
#include "physics/Raycast.hpp"

using namespace sc;

TEST_CASE("ray_vs_aabb hits front face with correct normal") {
    AABB box{{5.0f, 0.0f}, {1.0f, 1.0f}}; // x in [4,6]
    float t = 0.0f;
    glm::vec2 n{0.0f, 0.0f};
    REQUIRE(ray_vs_aabb({0.0f, 0.0f}, {1.0f, 0.0f}, box, 100.0f, t, n));
    CHECK(t == doctest::Approx(4.0f));      // reaches left face at x=4
    CHECK(n.x == doctest::Approx(-1.0f));   // outward normal faces the ray
}

TEST_CASE("ray_vs_aabb misses when aimed away") {
    AABB box{{5.0f, 0.0f}, {1.0f, 1.0f}};
    float t = 0.0f;
    glm::vec2 n{0.0f, 0.0f};
    CHECK_FALSE(ray_vs_aabb({0.0f, 0.0f}, {-1.0f, 0.0f}, box, 100.0f, t, n));
    CHECK_FALSE(ray_vs_aabb({0.0f, 0.0f}, {1.0f, 0.0f}, box, 3.0f, t, n)); // too short
}

TEST_CASE("ray_vs_circle hits near side") {
    Circle c{{5.0f, 0.0f}, 1.0f};
    float t = 0.0f;
    glm::vec2 n{0.0f, 0.0f};
    REQUIRE(ray_vs_circle({0.0f, 0.0f}, {1.0f, 0.0f}, c, 100.0f, t, n));
    CHECK(t == doctest::Approx(4.0f));
    CHECK(n.x == doctest::Approx(-1.0f));
}

TEST_CASE("raycast returns the nearest masked entity") {
    Registry reg;
    auto wall = [&](float x) {
        const Entity e = reg.create();
        reg.emplace<Transform>(e, Transform{{x, 0.0f}, 0.0f});
        Hitbox hb;
        hb.shape = HitboxShape::AABB;
        hb.halfExtents = {1.0f, 1.0f};
        reg.emplace<Hitbox>(e, hb);
        reg.emplace<CollisionLayer>(e, CollisionLayer{layer::Wall, 0u});
        return e;
    };
    const Entity near = wall(5.0f);
    wall(10.0f);

    const RayHit hit = raycast(reg, {0.0f, 0.0f}, {1.0f, 0.0f}, 100.0f, layer::Wall);
    REQUIRE(hit.hit);
    CHECK(hit.entity == near);
    CHECK(hit.t == doctest::Approx(4.0f));

    // Mask that matches nothing → no hit.
    CHECK_FALSE(raycast(reg, {0.0f, 0.0f}, {1.0f, 0.0f}, 100.0f, layer::Ship).hit);
}
