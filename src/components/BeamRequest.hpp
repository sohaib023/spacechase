#pragma once

// A ship's beam-fire intent for this step, set by fire_step and consumed by
// beam_step. Stores primitive beam params (not BeamParams) so this component
// stays in the low layer with no dependency on gameplay/Abilities.

#include <glm/glm.hpp>

namespace sc {

struct BeamRequest {
    bool active = false;     // fire2 held this step
    float damage = 0.0f;     // damage PER SECOND (continuous; scaled by dt)
    float maxDist = 30.0f;   // meters
    float width = 0.12f;     // meters (viz)
    glm::vec3 color{1.0f, 0.4f, 0.2f};
};

} // namespace sc
