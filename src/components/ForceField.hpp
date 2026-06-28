#pragma once

// A force-field SOURCE (brief §6.4): wind zones, deflectors, etc. all share this.
// Radial pushes along (target - source); Directional pushes along `dir`. Applies
// only to Deflectable targets within `range`. `strength` > 0 = push out / along dir.

#include <glm/glm.hpp>

namespace sc {

enum class FieldShape { Radial, Directional };
enum class FalloffType { None, Linear, InverseSquare };

struct ForceField {
    FieldShape shape = FieldShape::Radial;
    glm::vec2 dir{0.0f, -1.0f}; // directional only (normalized)
    float strength = 0.0f;      // accel magnitude (m/s^2)
    float range = 10.0f;        // meters
    FalloffType falloff = FalloffType::Linear;
};

} // namespace sc
