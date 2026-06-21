#pragma once

// 2D orthographic camera. Header-only math helper (brief: math/render layer).
// World units are METERS (+X right, +Y up). `pixelsPerMeter` sets how many
// screen pixels one world meter occupies at zoom 1; `zoom` > 1 magnifies.
// The camera is centered on `position` (meters).

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sc {

struct Camera2D {
    glm::vec2 position{0.0f, 0.0f}; // meters
    float zoom = 1.0f;
    float pixelsPerMeter = 48.0f;

    // Build the world(meters)->clip matrix for the given framebuffer size (pixels).
    glm::mat4 view_proj(float viewportW, float viewportH) const {
        // Visible half-extent in meters = (viewport pixels / 2) / (px per meter).
        const float halfW = (viewportW * 0.5f) / (pixelsPerMeter * zoom);
        const float halfH = (viewportH * 0.5f) / (pixelsPerMeter * zoom);
        // +Y up: bottom < top. Depth range is irrelevant for 2D; keep [-1,1].
        const glm::mat4 proj = glm::ortho(-halfW, halfW, -halfH, halfH, -1.0f, 1.0f);
        const glm::mat4 view =
            glm::translate(glm::mat4(1.0f), glm::vec3(-position.x, -position.y, 0.0f));
        return proj * view;
    }
};

} // namespace sc
