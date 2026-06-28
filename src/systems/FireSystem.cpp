#include "systems/FireSystem.hpp"

#include "components/BeamRequest.hpp"
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

        for (AbilitySlot& slot : loadout.slots) {
            slot.cooldownRemaining = std::max(0.0f, slot.cooldownRemaining - dt);
            const bool held = (slot.action == "fire2") ? actions.fire2 : actions.fire1;

            // Beam abilities set a BeamRequest for beam_step (continuous hitscan),
            // rather than queuing discrete spawns.
            if (slot.ability.category == AbilityCategory::Beam) {
                if (BeamRequest* beam = registry.try_get<BeamRequest>(shooter)) {
                    beam->active = held;
                    beam->damage = slot.ability.beam.damage;
                    beam->maxDist = slot.ability.beam.maxDist;
                    beam->width = slot.ability.beam.width;
                    beam->color = slot.ability.beam.color;
                }
                slot.wasHeld = held;
                continue;
            }

            // Fold the ability's defaults through its modifiers into a FireContext.
            FireContext context;
            for (const ModifierDef& modifier : slot.modifiers) {
                apply_modifier(context, modifier);
            }

            // Autofire holds to fire; otherwise one shot per press (rising edge).
            // Either way the cooldown gates the cadence.
            const bool trigger = context.autofire ? held : (held && !slot.wasHeld);
            if (trigger && slot.cooldownRemaining <= 0.0f) {
                const ProjectileParams& params = slot.ability.projectile;
                const float speed = params.speed * context.speedMult;
                for (int shot = 0; shot < context.count; ++shot) {
                    const float offset =
                        context.count > 1
                            ? -context.spread * 0.5f +
                                  context.spread * static_cast<float>(shot) /
                                      static_cast<float>(context.count - 1)
                            : 0.0f;
                    const float rotation = transform.rotation + offset;
                    const glm::vec2 dir{-std::sin(rotation), std::cos(rotation)};
                    ProjectileSpawn spawn;
                    spawn.params = params;
                    spawn.albedo = slot.projAlbedo;
                    spawn.normal = slot.projNormal;
                    spawn.position = transform.position + dir * kMuzzleOffset;
                    spawn.velocity = dir * speed;
                    spawn.rotation = rotation;
                    spawn.owner = shooter;
                    spawn.team = 0;
                    deferred.spawns.push_back(spawn);
                }
                slot.cooldownRemaining = slot.ability.cooldown * context.cooldownMult;
            }
            slot.wasHeld = held;
        }
    }
}

} // namespace sc
