#pragma once

// Pseudo-3D vertical state. Ships hover at a height above the floor plane; over
// void, gravity pulls the altitude down (brief §6.6, single layer). `visualScale`
// is maintained by the floor system for a fall-away rendering cue.

namespace sc {

struct Altitude {
    float z = 1.0f;            // meters above the floor plane (hover height)
    float vz = 0.0f;           // vertical velocity (m/s)
    bool falling = false;      // true while airborne over void
    float visualScale = 1.0f;  // sprite scale cue (1 grounded → smaller as it falls)
};

} // namespace sc
