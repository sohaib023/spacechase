#pragma once

// Seconds an entity has left to live; the lifetime system decrements it and
// queues despawn at <= 0 (projectiles, transient effects).

namespace sc {

struct Lifetime {
    float remaining = 1.0f;
};

} // namespace sc
