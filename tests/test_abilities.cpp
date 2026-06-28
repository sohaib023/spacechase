#include <doctest/doctest.h>

#include "components/PlayerControlled.hpp"
#include "components/Transform.hpp"
#include "core/Input.hpp"
#include "ecs/Registry.hpp"
#include "gameplay/Abilities.hpp"
#include "gameplay/Spawning.hpp"
#include "systems/FireSystem.hpp"

#include <utility>
#include <vector>

using namespace sc;

// --- The modifier fold (pure) -------------------------------------------------

TEST_CASE("multifire adds shots and sets the spread") {
    FireContext context;
    apply_modifier(context, ModifierDef{ModifierKind::Multifire, 2.0f, 0.3f});
    CHECK(context.count == 3);
    CHECK(context.spread == doctest::Approx(0.3f));
}

TEST_CASE("quickfire scales cooldown and (optionally) speed") {
    FireContext context;
    apply_modifier(context, ModifierDef{ModifierKind::Quickfire, 0.5f, 2.0f});
    CHECK(context.cooldownMult == doctest::Approx(0.5f));
    CHECK(context.speedMult == doctest::Approx(2.0f));
}

TEST_CASE("autofire sets the hold-to-fire flag") {
    FireContext context;
    apply_modifier(context, ModifierDef{ModifierKind::Autofire, 0.0f, 0.0f});
    CHECK(context.autofire);
}

TEST_CASE("modifiers stack; quickfire with b=0 leaves speed untouched") {
    FireContext context;
    apply_modifier(context, ModifierDef{ModifierKind::Multifire, 1.0f, 0.2f});
    apply_modifier(context, ModifierDef{ModifierKind::Quickfire, 0.25f, 0.0f});
    CHECK(context.count == 2);
    CHECK(context.cooldownMult == doctest::Approx(0.25f));
    CHECK(context.speedMult == doctest::Approx(1.0f));
}

// --- Modifiers through fire_step ---------------------------------------------

namespace {
constexpr float kDt = 1.0f / 60.0f;

Entity shooter_with(Registry& reg, std::vector<ModifierDef> modifiers, float cooldown) {
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{{0.0f, 0.0f}, 0.0f});
    reg.emplace<PlayerControlled>(e);
    AbilityLoadout loadout;
    AbilitySlot slot;
    slot.ability.category = AbilityCategory::Projectile;
    slot.ability.cooldown = cooldown;
    slot.ability.projectile.speed = 10.0f;
    slot.modifiers = std::move(modifiers);
    loadout.slots.push_back(slot);
    reg.emplace<AbilityLoadout>(e, loadout);
    return e;
}
} // namespace

TEST_CASE("multifire emits count shots in one trigger, angularly spread") {
    Registry reg;
    Deferred deferred;
    shooter_with(reg, {ModifierDef{ModifierKind::Multifire, 2.0f, 0.4f}}, 0.5f);
    InputActions actions;
    actions.fire1 = true;
    fire_step(reg, actions, deferred, kDt);
    REQUIRE(deferred.spawns.size() == 3);
    // Outer shots are rotated +/- spread/2, so their x-velocities differ.
    CHECK(deferred.spawns.front().velocity.x != doctest::Approx(deferred.spawns.back().velocity.x));
}

TEST_CASE("quickfire shortens the cooldown set after firing") {
    Registry reg;
    Deferred deferred;
    const Entity e = shooter_with(reg, {ModifierDef{ModifierKind::Quickfire, 0.5f, 0.0f}}, 0.2f);
    InputActions actions;
    actions.fire1 = true;
    fire_step(reg, actions, deferred, kDt);
    CHECK(reg.get<AbilityLoadout>(e).slots[0].cooldownRemaining == doctest::Approx(0.1f));
}

TEST_CASE("semi-auto fires once per press; autofire fires repeatedly") {
    {
        Registry reg;
        Deferred deferred;
        shooter_with(reg, {}, 0.05f); // no autofire -> rising edge
        InputActions actions;
        actions.fire1 = true;
        for (int i = 0; i < 30; ++i) {
            fire_step(reg, actions, deferred, kDt); // held the whole time
        }
        CHECK(deferred.spawns.size() == 1);
    }
    {
        Registry reg;
        Deferred deferred;
        shooter_with(reg, {ModifierDef{ModifierKind::Autofire, 0.0f, 0.0f}}, 0.05f);
        InputActions actions;
        actions.fire1 = true;
        for (int i = 0; i < 30; ++i) {
            fire_step(reg, actions, deferred, kDt);
        }
        CHECK(deferred.spawns.size() > 1);
    }
}
