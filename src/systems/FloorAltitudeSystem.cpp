#include "systems/FloorAltitudeSystem.hpp"

#include "components/Altitude.hpp"
#include "components/ShipControl.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "core/Log.hpp"
#include "gameplay/FallConfig.hpp"
#include "gameplay/Level.hpp"

#include <algorithm>

namespace sc {

void apply_fall_control(Registry& registry, const FallConfig& config) {
    if (config.controlWhileFalling) {
        return;
    }
    auto view = registry.view<ShipControl, Altitude>();
    for (const Entity entity : view) {
        if (view.get<Altitude>(entity).falling) {
            // Momentum carries the ship; no thrust. (Rotation is still allowed.)
            view.get<ShipControl>(entity).thrust = 0.0f;
        }
    }
}

void floor_altitude_step(Registry& registry, const Level& level, const FallConfig& config,
                         glm::vec2 spawn, float dt) {
    auto view = registry.view<Transform, Velocity, Altitude>();
    for (const Entity entity : view) {
        Transform& transform = view.get<Transform>(entity);
        Velocity& velocity = view.get<Velocity>(entity);
        Altitude& altitude = view.get<Altitude>(entity);

        if (level.has_floor(transform.position)) {
            // Over floor: settle back up to hover height (recovery) and ground out.
            altitude.vz = 0.0f;
            if (altitude.z < config.hoverHeight) {
                altitude.z = std::min(config.hoverHeight, altitude.z + config.recoverRate * dt);
            } else {
                altitude.z = config.hoverHeight;
            }
            altitude.falling = altitude.z < config.hoverHeight - 1e-3f;
        } else {
            // Over void: gravity pulls the ship down.
            altitude.vz -= config.gravity * dt;
            altitude.z += altitude.vz * dt;
            altitude.falling = true;
            if (altitude.z <= config.deathZ) {
                // Fatal fall — respawn at the spawn point (placeholder for M3).
                transform.position = spawn;
                velocity.value = glm::vec2(0.0f);
                altitude = Altitude{config.hoverHeight, 0.0f, false, 1.0f};
                SC_LOG_INFO("Ship fell into the void — respawned.");
                continue;
            }
        }

        // Fall-away visual cue: full size grounded, shrinking toward death depth.
        const float span = config.hoverHeight - config.deathZ;
        const float frac = span > 1e-3f ? (altitude.z - config.deathZ) / span : 1.0f;
        altitude.visualScale = 0.4f + 0.6f * std::clamp(frac, 0.0f, 1.0f);
    }
}

} // namespace sc
