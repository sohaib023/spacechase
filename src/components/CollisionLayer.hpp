#pragma once

// Bitmask collision filtering: an entity is on `layer` and only collides with
// entities whose layer is in its `mask`. Lets broadphase skip non-interacting
// pairs and future-proofs projectiles/teams (M5) without reworking the system.

#include <cstdint>

namespace sc {

namespace layer {
inline constexpr std::uint32_t Ship = 1u << 0;
inline constexpr std::uint32_t Wall = 1u << 1;
inline constexpr std::uint32_t Projectile = 1u << 2;
// Pickup, ... added as needed.
} // namespace layer

struct CollisionLayer {
    std::uint32_t layer = 0; // what this entity is
    std::uint32_t mask = 0;  // which layers it resolves against
};

} // namespace sc
