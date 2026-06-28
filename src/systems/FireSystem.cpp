#include "systems/FireSystem.hpp"

#include "components/PlayerControlled.hpp"
#include "components/Transform.hpp"
#include "core/Input.hpp"
#include "gameplay/Abilities.hpp"
#include "gameplay/Spawning.hpp"

#include <algorithm>
#include <cmath>

namespace sc {

namespace {
// Muzzle offset ahead of the hull so the bullet spawns clear of the ship's own
// hitbox (and clear of the owner self-hit filter's segment start).
constexpr float kMuzzleOffset = 1.0f;
} // namespace

void fire_step(Registry& registry, const InputActions& actions, Deferred& deferred, float dt) {
    auto view = registry.view<AbilityLoadout, Transform, PlayerControlled>();
    for (const Entity shooter : view) {
        AbilityLoadout& loadout = view.get<AbilityLoadout>(shooter);
        const Transform& transform = view.get<Transform>(shooter);
        const glm::vec2 forward{-std::sin(transform.rotation), std::cos(transform.rotation)};

        for (AbilitySlot& slot : loadout.slots) {
            slot.cooldownRemaining = std::max(0.0f, slot.cooldownRemaining - dt);
            const bool held = (slot.action == "fire2") ? actions.fire2 : actions.fire1;

            // Beam abilities are driven by beam_step (M5.6), not the spawn queue.
            if (slot.ability.category != AbilityCategory::Projectile) {
                slot.wasHeld = held;
                continue;
            }

            if (held && slot.cooldownRemaining <= 0.0f) {
                const ProjectileParams& params = slot.ability.projectile;
                ProjectileSpawn spawn;
                spawn.params = params;
                spawn.albedo = slot.projAlbedo;
                spawn.normal = slot.projNormal;
                spawn.position = transform.position + forward * kMuzzleOffset;
                spawn.velocity = forward * params.speed;
                spawn.rotation = transform.rotation;
                spawn.owner = shooter;
                spawn.team = 0;
                deferred.spawns.push_back(spawn);
                slot.cooldownRemaining = slot.ability.cooldown;
            }
            slot.wasHeld = held;
        }
    }
}

} // namespace sc
