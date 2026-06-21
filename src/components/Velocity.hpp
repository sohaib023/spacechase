#pragma once

// Linear velocity in world units per second. Integrated by the movement system.

#include <glm/glm.hpp>

namespace sc {

struct Velocity {
    glm::vec2 value{0.0f, 0.0f};
};

} // namespace sc
