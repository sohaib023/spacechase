#pragma once

#include "ecs/Registry.hpp"

namespace sc {

struct InputActions;
struct Deferred;

// Tick cooldowns and, for each PlayerControlled ship's loadout, queue projectile
// spawns when its fire action is active and off cooldown. Queues only (never
// creates entities) — commit_deferred applies them. Beam abilities are handled by
// beam_step (M5.6), not here.
void fire_step(Registry& registry, const InputActions& actions, Deferred& deferred, float dt);

} // namespace sc
