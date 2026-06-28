#include <doctest/doctest.h>

#include "components/Lifetime.hpp"
#include "ecs/Registry.hpp"
#include "gameplay/Spawning.hpp"
#include "systems/LifetimeSystem.hpp"

using namespace sc;

namespace {
constexpr float kDt = 1.0f / 60.0f;
}

TEST_CASE("lifetime decrements and queues a despawn when it elapses") {
    Registry reg;
    Deferred deferred;
    const Entity e = reg.create();
    reg.emplace<Lifetime>(e, Lifetime{0.05f});

    lifetime_step(reg, deferred, kDt);
    CHECK(reg.get<Lifetime>(e).remaining < 0.05f);
    CHECK(deferred.despawns.empty());

    lifetime_step(reg, deferred, kDt);
    lifetime_step(reg, deferred, kDt); // total ~0.05 s -> expired
    CHECK(deferred.despawns.size() >= 1);
}

TEST_CASE("commit_deferred destroys queued despawns and clears the queue") {
    Registry reg;
    Deferred deferred;
    const Entity e = reg.create();
    reg.emplace<Lifetime>(e, Lifetime{0.0f}); // already expired
    lifetime_step(reg, deferred, kDt);
    REQUIRE(deferred.despawns.size() == 1);
    commit_deferred(reg, deferred);
    CHECK_FALSE(reg.valid(e));
    CHECK(deferred.despawns.empty());
}

TEST_CASE("commit_deferred tolerates duplicate despawns (no double-destroy)") {
    Registry reg;
    Deferred deferred;
    const Entity e = reg.create();
    deferred.despawns.push_back(e);
    deferred.despawns.push_back(e); // duplicate (hit + expired same step)
    commit_deferred(reg, deferred);
    CHECK_FALSE(reg.valid(e));
}
