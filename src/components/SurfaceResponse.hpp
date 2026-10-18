#pragma once

// How a surface (wall, crate, ...) treats projectiles and beams that hit it.
// Absorbent surfaces stop them; Reflective surfaces bounce them, but only if the
// projectile/beam itself is bouncy. Explicit on every wall so the choice is
// declared data, not the absence of a tag.

namespace sc {

enum class SurfaceType { Absorbent, Reflective };

struct SurfaceResponse {
    SurfaceType type = SurfaceType::Absorbent;
};

} // namespace sc
