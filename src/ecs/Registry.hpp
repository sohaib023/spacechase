#pragma once

// Single include seam for EnTT, so the rest of the codebase refers to sc::Registry
// / sc::Entity and the specific ECS library stays swappable in one place.

#include <entt/entt.hpp>

namespace sc {

using Registry = entt::registry;
using Entity = entt::entity;

} // namespace sc
