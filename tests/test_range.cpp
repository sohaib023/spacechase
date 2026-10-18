#include <doctest/doctest.h>

#include "components/Range.hpp"
#include "components/Velocity.hpp"
#include "ecs/Registry.hpp"
#include "gameplay/Spawning.hpp"
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
