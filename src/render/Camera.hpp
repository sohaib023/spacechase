#pragma once

// 2D orthographic camera. Header-only math helper (brief: math/render layer).
// World units: for M1, 1 world unit = 1 pixel at zoom 1; +X right, +Y up.
// The camera is centered on `position`; `zoom` > 1 magnifies.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sc {

struct Camera2D {
    glm::vec2 position{0.0f, 0.0f};
    float zoom = 1.0f;

    // Build the world->clip matrix for the given framebuffer size (pixels).
    glm::mat4 view_proj(float viewportW, float viewportH) const {
        const float halfW = (viewportW * 0.5f) / zoom;
        const float halfH = (viewportH * 0.5f) / zoom;
        // +Y up: bottom < top. Depth range is irrelevant for 2D; keep [-1,1].
        const glm::mat4 proj = glm::ortho(-halfW, halfW, -halfH, halfH, -1.0f, 1.0f);
        const glm::mat4 view =
            glm::translate(glm::mat4(1.0f), glm::vec3(-position.x, -position.y, 0.0f));
        return proj * view;
    }
};

} // namespace sc
