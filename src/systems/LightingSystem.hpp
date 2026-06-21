#pragma once

// Collects all light entities into a LightsUBO for the fragment shader. Runs at
// render time (lighting is presentation, not fixed-step simulation).

#include "ecs/Registry.hpp"
#include "render/LightsUBO.hpp"

#include <glm/glm.hpp>

namespace sc {

// `ambient` = rgb + strength; `params` = (shininess, specularStrength, _, _).
LightsUBO collect_lights(const Registry& registry, glm::vec4 ambient, glm::vec4 params);

} // namespace sc
