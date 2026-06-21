#pragma once

// Input system: writes the local player's abstract InputActions into the
// ShipControl of every PlayerControlled entity. The single point where raw
// input meets the ECS (AI / network writers target ShipControl the same way).

#include "core/Input.hpp"
#include "ecs/Registry.hpp"

namespace sc {

void input_apply(Registry& registry, const InputActions& actions);

} // namespace sc
