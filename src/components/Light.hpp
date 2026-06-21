#pragma once

// A light source. Attached to an entity with a Transform (point lights use the
// transform's position). Each light has a HEIGHT above the sprite plane — the
// key 2.5D knob (brief §6.7). Lights are collected at render time, not simulated.

#include <glm/glm.hpp>

namespace sc {

enum class LightType { Point = 0, Directional = 1 };

struct Light {
    LightType type = LightType::Point;
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float height = 2.0f;             // meters above the plane
    float range = 10.0f;             // point-light falloff radius (meters)
    glm::vec2 direction{0.0f, -1.0f}; // directional lights (in-plane)
};

} // namespace sc
