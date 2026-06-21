#pragma once

// Abstract input actions, decoupled from raw keys (brief §5: this layer is what
// makes a touch/gamepad port cheap — only the mapping changes, not gameplay).
// M1 has the two actions a ship needs; this grows into a proper action map later.

namespace sc {

struct InputActions {
    float thrust = 0.0f; // +1 forward, -1 reverse
    float turn = 0.0f;   // +1 turn left (CCW), -1 turn right (CW)
};

} // namespace sc
