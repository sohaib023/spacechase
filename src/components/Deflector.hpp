#pragma once

// The ship's deflector defensive ability: while `active`, the ship becomes a
// radial force-field source (reusing the force-field system) that repels
// Deflectable objects (incoming projectiles, other ships). Toggled in debug UI.

namespace sc {

struct Deflector {
    bool active = false;
    float strength = 40.0f; // outward push (m/s^2) at the center
    float range = 5.0f;     // meters
};

} // namespace sc
