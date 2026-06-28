#pragma once

// Deferred structural changes (brief §8): entity create/destroy NEVER happens
// mid-iteration (it invalidates EnTT views and breaks determinism). Systems queue
// onto these lists; commit_deferred is the ONE place that applies them, at the end
// of the fixed step.

#include "ecs/Registry.hpp"
#include "gameplay/Abilities.hpp" // ProjectileParams

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sc {

struct ProjectileSpawn {
    ProjectileParams params;
    TextureHandle albedo = kInvalidTexture;
    TextureHandle normal = kInvalidTexture;
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 velocity{0.0f, 0.0f};
    float rotation = 0.0f;        // facing radians (0 = +Y) for the sprite
    Entity owner = entt::null;    // shooter (self-hit filter)
    std::uint32_t team = 0;
};

struct Deferred {
    std::vector<ProjectileSpawn> spawns;
    std::vector<Entity> despawns;
};

// Flush queued spawns then despawns, then clear. The only structural-change point.
void commit_deferred(Registry& registry, Deferred& deferred);

} // namespace sc
