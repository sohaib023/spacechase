#pragma once

// GPU-facing lights uniform buffer. Byte layout MUST match the `LightsUBO`
// cbuffer in assets/shaders/lit_sprite.hlsl exactly (each member is a 16-byte
// float4 lane; the light array elements are 48 bytes = 3 lanes, 16-aligned).
// Lives in the render layer (it's a render resource); systems fill it.

#include <glm/glm.hpp>

namespace sc {

inline constexpr int kMaxLights = 16;

struct GpuLight {
    glm::vec4 posHeight; // xy = world pos, z = height, w = type (0 point, 1 dir)
    glm::vec4 color;     // rgb = color, a = intensity
    glm::vec4 rangeDir;  // x = range, yz = direction, w = unused
};

struct LightsUBO {
    glm::vec4 ambient{0.0f, 0.0f, 0.0f, 0.0f}; // rgb, a = strength
    glm::vec4 params{16.0f, 0.0f, 0.0f, 0.0f}; // x = shininess, y = specular strength
    glm::ivec4 counts{0, 0, 0, 0};             // x = active light count
    GpuLight lights[kMaxLights]{};
};

static_assert(sizeof(LightsUBO) == 16 * 3 + kMaxLights * 48,
              "LightsUBO must match the HLSL cbuffer byte layout");

} // namespace sc
