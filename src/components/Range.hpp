#pragma once

// Meters of travel an entity has left; the range system decrements it by the
// distance covered each step and queues despawn at <= 0. Independent of bouncing.

namespace sc {

struct Range {
    float remaining = 40.0f;
};

} // namespace sc
