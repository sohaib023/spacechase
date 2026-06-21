#pragma once

// Render-collect system: interpolates each drawable entity's transform and
// queues it into the sprite batch. Replaces M1's inline draw block.

#include "ecs/Registry.hpp"

namespace sc {

class Renderer;

// `alpha` is the fixed-step interpolation factor in [0,1].
void render_collect(Registry& registry, Renderer& renderer, float alpha);

} // namespace sc
