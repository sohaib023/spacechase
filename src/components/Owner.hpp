#pragma once

// Who spawned this entity (e.g. which ship fired a projectile). Used for
// self-hit / friendly-fire filtering. `team` is reserved for future PvP.

#include "ecs/Registry.hpp"

#include <cstdint>

namespace sc {

struct Owner {
    Entity entity = entt::null;
    std::uint32_t team = 0;
};

} // namespace sc
