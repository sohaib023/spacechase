#pragma once

// Per-entity movement tuning, populated from a ship definition (data-driven).
// Was M1's ShipConfig; now a component so each ship can differ.

namespace sc {

struct ShipStats {
    float acceleration = 0.0f; // world units / s^2 along facing
    float maxSpeed = 0.0f;     // world units / s
    float turnSpeed = 0.0f;    // radians / s
    float drag = 0.0f;         // linear damping coefficient (per second)
};

} // namespace sc
