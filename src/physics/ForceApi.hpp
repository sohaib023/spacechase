#pragma once

// Seam for M5's unified force-field system (wind / deflectors / chi-push, brief
// §6.4): a force field is a shape + a force function; the force system applies
// impulses to entities with Velocity through this API. Only the impulse helper
// exists now — fields themselves are M5. Kept here so the physics layer owns the
// force-application concept from the start.

#include <glm/glm.hpp>

namespace sc {

// Apply an instantaneous velocity change (impulse already divided by mass).
inline void apply_impulse(glm::vec2& velocity, glm::vec2 impulse) {
    velocity += impulse;
}

} // namespace sc
