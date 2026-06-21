#pragma once

// What the render-collect system needs to draw an entity. M2 holds a raw
// pointer into an App-owned Texture (which outlives the registry); this becomes
// a resource handle once a texture registry exists (M3).

#include "resources/Texture.hpp"

#include <glm/glm.hpp>

namespace sc {

struct SpriteRender {
    const Texture* texture = nullptr;
    glm::vec2 size{64.0f, 64.0f}; // world-unit size of one frame
    int frameCount = 1;           // horizontal frames in the sheet
    int frame = 0;                // currently selected frame
};

} // namespace sc
