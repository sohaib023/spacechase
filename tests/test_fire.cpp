#include <doctest/doctest.h>

#include "components/PlayerControlled.hpp"
#include "components/Transform.hpp"
#include "core/Input.hpp"
#include "ecs/Registry.hpp"
#include "gameplay/Abilities.hpp"
#include "gameplay/Spawning.hpp"
#include "systems/FireSystem.hpp"

using namespace sc;

namespace {
constexpr float kDt = 1.0f / 60.0f;

Entity make_shooter(Registry& reg, AbilityCategory category, float cooldown) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{{0.0f, 0.0f}, 0.0f});
    reg.emplace<PlayerControlled>(e);
    AbilityLoadout loadout;
    AbilitySlot slot;
    slot.ability.category = category;
    slot.ability.cooldown = cooldown;
    slot.ability.projectile.speed = 10.0f;
    slot.action = "fire1";
    loadout.slots.push_back(slot);
    reg.emplace<AbilityLoadout>(e, loadout);
    return e;
}
} // namespace

TEST_CASE("fires on the first held step and goes on cooldown") {
    Registry reg;
    Deferred deferred;
    const Entity e = make_shooter(reg, AbilityCategory::Projectile, 0.5f);
    InputActions actions;
    actions.fire1 = true;
    fire_step(reg, actions, deferred, kDt);
    CHECK(deferred.spawns.size() == 1);
    CHECK(reg.get<AbilityLoadout>(e).slots[0].cooldownRemaining == doctest::Approx(0.5f));
}

TEST_CASE("holds fire on cooldown, then fires again once it elapses") {
    Registry reg;
    Deferred deferred;
    make_shooter(reg, AbilityCategory::Projectile, 0.2f);
    InputActions actions;
    actions.fire1 = true;
    fire_step(reg, actions, deferred, kDt); // shot 1 (cooldown -> 0.2)
    fire_step(reg, actions, deferred, kDt); // still cooling
    CHECK(deferred.spawns.size() == 1);
    for (int i = 0; i < 12; ++i) {
        fire_step(reg, actions, deferred, kDt); // ~0.2 s elapses -> shot 2
    }
    CHECK(deferred.spawns.size() == 2);
}

TEST_CASE("does not fire while the button is up") {
    Registry reg;
    Deferred deferred;
    make_shooter(reg, AbilityCategory::Projectile, 0.1f);
    InputActions actions; // fire1 = false
    for (int i = 0; i < 30; ++i) {
        fire_step(reg, actions, deferred, kDt);
    }
    CHECK(deferred.spawns.empty());
}

TEST_CASE("beam abilities never queue projectile spawns") {
    Registry reg;
    Deferred deferred;
    make_shooter(reg, AbilityCategory::Beam, 0.0f);
    InputActions actions;
    actions.fire1 = true;
    for (int i = 0; i < 10; ++i) {
        fire_step(reg, actions, deferred, kDt);
    }
    CHECK(deferred.spawns.empty());
}

TEST_CASE("queued spawn carries the owner and a +Y muzzle velocity at facing 0") {
    Registry reg;
    Deferred deferred;
    const Entity e = make_shooter(reg, AbilityCategory::Projectile, 0.5f);
    InputActions actions;
    actions.fire1 = true;
    fire_step(reg, actions, deferred, kDt);
    REQUIRE(deferred.spawns.size() == 1);
    CHECK(deferred.spawns[0].owner == e);
    CHECK(deferred.spawns[0].velocity.x == doctest::Approx(0.0f));
    CHECK(deferred.spawns[0].velocity.y == doctest::Approx(10.0f)); // facing 0 = +Y
}
