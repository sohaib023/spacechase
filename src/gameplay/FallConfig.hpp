#pragma once

// Tunables for the pseudo-3D falling model. `controlWhileFalling` is the flag
// the user asked to be easily flippable: on = steer-to-recover, off = momentum
// carries you (no thrust while falling).

namespace sc {

struct FallConfig {
    float gravity = 18.0f;            // m/s^2 pulling altitude down over void
    float hoverHeight = 1.0f;         // resting altitude above the floor plane (m)
    float deathZ = 0.0f;              // altitude at/below which a fall is fatal (m)
    float recoverRate = 6.0f;         // m/s the ship rises back to hover on recovery
    bool controlWhileFalling = true;  // can the player thrust while falling?
};

} // namespace sc
