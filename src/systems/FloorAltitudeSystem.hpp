#pragma once

// Pseudo-3D falling: ships hover at FallConfig.hoverHeight over the floor; over
// void, gravity lowers their altitude. Return over floor before altitude hits
// deathZ → recover; reach deathZ over void → fall is fatal (respawn at `spawn`
// in M3). Single layer — no dropping to a lower floor (deferred, brief §10.5).

#include "ecs/Registry.hpp"

#include <glm/glm.hpp>

namespace sc {

struct Level;
struct FallConfig;

// Lock horizontal thrust for falling ships when controlWhileFalling is off.
// Call before movement_step (uses the falling flag from the previous step).
void apply_fall_control(Registry& registry, const FallConfig& config);

// Advance vertical state, handle recovery/death. Call after collision_step.
void floor_altitude_step(Registry& registry, const Level& level, const FallConfig& config,
                         glm::vec2 spawn, float dt);

} // namespace sc
