#pragma once

// Per-entity control intent for the current step. Written by whatever drives the
// entity (local input now; AI / network later) and read by the movement system.
// Decouples "who decides" from "how it moves".

namespace sc {

struct ShipControl {
    float thrust = 0.0f; // +1 forward, -1 reverse
    float turn = 0.0f;   // +1 turn left (CCW), -1 turn right (CW)
};

} // namespace sc
