#pragma once

// Every collidable object declares how it responds to force fields: Rigid objects
// (walls, crates) are never moved; Deflectable objects (ships, projectiles) get
// pushed. Explicit on all entities so "rigid" is a declared choice, not the
// implicit absence of a tag.

namespace sc {

enum class ForceResponseType { Rigid, Deflectable };

struct ForceResponse {
    ForceResponseType type = ForceResponseType::Rigid;
};

} // namespace sc
