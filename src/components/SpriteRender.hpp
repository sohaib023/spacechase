#pragma once

// What the render-collect system needs to draw an entity. Holds a TextureHandle
// into the TextureRegistry (M3) rather than a raw pointer.

#include "resources/TextureHandle.hpp"

#include <glm/glm.hpp>

namespace sc {

struct SpriteRender {
    TextureHandle texture = kInvalidTexture;
    glm::vec2 size{1.0f, 1.0f}; // world size (meters) of one frame
    int frameCount = 1;         // horizontal frames in the sheet
    int frame = 0;              // currently selected frame
};

} // namespace sc
