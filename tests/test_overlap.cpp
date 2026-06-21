#include <doctest/doctest.h>

#include "physics/Overlap.hpp"

using namespace sc;

TEST_CASE("circle_vs_aabb: separated → no hit") {
    Circle c{{5.0f, 0.0f}, 1.0f};
    AABB b{{0.0f, 0.0f}, {1.0f, 1.0f}};
    CHECK_FALSE(circle_vs_aabb(c, b).hit);
}

TEST_CASE("circle_vs_aabb: overlap from the right gives +x normal") {
    // Box spans x[-1,1]; circle center at x=1.5, r=1 → overlaps right face by 0.5.
    Circle c{{1.5f, 0.0f}, 1.0f};
    AABB b{{0.0f, 0.0f}, {1.0f, 1.0f}};
    const Manifold m = circle_vs_aabb(c, b);
    REQUIRE(m.hit);
    CHECK(m.normal.x == doctest::Approx(1.0f));
    CHECK(m.normal.y == doctest::Approx(0.0f));
    CHECK(m.penetration == doctest::Approx(0.5f));
}

TEST_CASE("circle_vs_aabb: center inside resolves out nearest face") {
    // Center just left of middle → nearest face is left (−x); penetration adds radius.
    Circle c{{-0.6f, 0.0f}, 0.5f};
    AABB b{{0.0f, 0.0f}, {1.0f, 1.0f}};
    const Manifold m = circle_vs_aabb(c, b);
    REQUIRE(m.hit);
    CHECK(m.normal.x == doctest::Approx(-1.0f));
    CHECK(m.penetration == doctest::Approx((1.0f - 0.6f) + 0.5f)); // distToLeftFace + r
}

TEST_CASE("circle_vs_circle") {
    CHECK(circle_vs_circle({{0, 0}, 1.0f}, {{1.5f, 0}, 1.0f}).hit);          // overlap 0.5
    CHECK(circle_vs_circle({{0, 0}, 1.0f}, {{1.5f, 0}, 1.0f}).penetration ==
          doctest::Approx(0.5f));
    CHECK_FALSE(circle_vs_circle({{0, 0}, 1.0f}, {{3.0f, 0}, 1.0f}).hit);    // separated
}

TEST_CASE("aabb_vs_aabb") {
    CHECK(aabb_vs_aabb({{0, 0}, {1, 1}}, {{1.5f, 0}, {1, 1}}));
    CHECK_FALSE(aabb_vs_aabb({{0, 0}, {1, 1}}, {{3.0f, 0}, {1, 1}}));
}
