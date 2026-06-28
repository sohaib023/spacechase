#pragma once

#include "ecs/Registry.hpp"

namespace sc {

struct Deferred;

// Swept hit detection for projectiles: raycast each projectile's PrevTransform->
// Transform segment against layer::Wall (tunneling-proof for fast bullets). On a
// hit (skipping the owner), apply Damage to the target's Health, queue the target
// for despawn if its Health drops to zero (destroys crates), and always queue the
// projectile for despawn. Queues only — commit_deferred applies them.
void projectile_hit_step(Registry& registry, Deferred& deferred);

} // namespace sc
