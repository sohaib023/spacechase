#pragma once

// Tag: this entity is a projectile (straight-line mover, hit-tested by a swept
// raycast each step). Speed lives in Velocity; damage/lifetime/owner are separate
// components.

namespace sc {

struct Projectile {};

} // namespace sc
