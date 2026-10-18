#pragma once

#include "ecs/Registry.hpp"

namespace sc {

struct Deferred;

// Swept hit detection for projectiles: raycast each projectile's PrevTransform->
// Transform segment against layer::Wall (tunneling-proof for fast bullets). On a
// hit (skipping the owner), apply Damage to the target's Health and queue the
// target for despawn if its Health drops to zero (destroys crates). Then, if the
// projectile has Bounce with bounces left and the surface is Reflective, it
// ricochets (velocity mirrored about the surface normal, same speed) and the rest
// of the step's path is swept again; otherwise it is queued for despawn.
// Queues only — commit_deferred applies them.
void projectile_hit_step(Registry& registry, Deferred& deferred);

} // namespace sc
