#pragma once

// Ship simulation state + the momentum/drag movement model (brief §6.2). M1
// keeps this as a plain struct updated by a free function; it migrates to ECS
// components + a movement system in M2.

#include "core/Input.hpp"

#include <glm/glm.hpp>

namespace sc {

// Tunable per-ship parameters (data-driven later; literals for now).
struct ShipConfig {
    float acceleration = 900.0f; // world units / s^2 along facing
    float maxSpeed = 600.0f;     // world units / s
    float turnSpeed = 3.2f;      // radians / s
    float drag = 1.2f;           // linear damping coefficient (per second)
};

struct ShipState {
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 velocity{0.0f, 0.0f};
    float facing = 0.0f; // radians; 0 = pointing +Y (up)
};

// Advance the ship by one fixed timestep.
void ship_step(ShipState& state, const ShipConfig& cfg, const InputActions& input, float dt);

// Interpolate between two states for rendering (t in [0,1]).
ShipState ship_lerp(const ShipState& a, const ShipState& b, float t);

} // namespace sc
