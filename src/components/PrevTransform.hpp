#pragma once

// Snapshot of an entity's Transform from the start of the current fixed step.
// The render pass interpolates PrevTransform -> Transform by the timestep alpha
// so motion stays smooth between simulation steps (brief §3.4).

#include <glm/glm.hpp>

namespace sc {

struct PrevTransform {
    glm::vec2 position{0.0f, 0.0f};
    float rotation = 0.0f;
};

} // namespace sc
