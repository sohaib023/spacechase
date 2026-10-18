#pragma once

// Marks a projectile as bouncy: it ricochets off Reflective surfaces instead of
// despawning. `remaining` counts bounces left; -1 means unlimited. No component
// means the projectile never bounces.

namespace sc {

struct Bounce {
    int remaining = -1;
};

} // namespace sc
