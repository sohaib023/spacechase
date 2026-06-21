#pragma once

// The simulation world: owns the entity registry plus the named entity handles
// the app cares about. Held by value in App (not a global), keeping the sim
// self-contained and replication-friendly (brief §8).

#include "ecs/Registry.hpp"

namespace sc {

struct World {
    Registry registry;
    Entity playerShip = entt::null;
};

} // namespace sc
