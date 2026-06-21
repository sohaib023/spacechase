#include "systems/MovementSystem.hpp"

#include "components/PrevTransform.hpp"
#include "components/ShipControl.hpp"
#include "components/ShipStats.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"

#include <cmath>

namespace sc {

void snapshot_prev_transforms(Registry& registry) {
    auto view = registry.view<Transform, PrevTransform>();
    for (const Entity entity : view) {
        const Transform& current = view.get<Transform>(entity);
        PrevTransform& prev = view.get<PrevTransform>(entity);
        prev.position = current.position;
        prev.rotation = current.rotation;
    }
}

void movement_step(Registry& registry, float dt) {
    auto view = registry.view<Transform, Velocity, ShipStats, ShipControl>();
    for (const Entity entity : view) {
        Transform& transform = view.get<Transform>(entity);
        Velocity& velocity = view.get<Velocity>(entity);
        const ShipStats& stats = view.get<ShipStats>(entity);
        const ShipControl& control = view.get<ShipControl>(entity);

        // Identical math to M1's ship_step(): turn, thrust along facing, drag,
        // clamp to max speed, integrate. (facing 0 = +Y up.)
        transform.rotation += control.turn * stats.turnSpeed * dt;
        const glm::vec2 forward{-std::sin(transform.rotation), std::cos(transform.rotation)};
        velocity.value += forward * (control.thrust * stats.acceleration * dt);
        velocity.value -= velocity.value * (stats.drag * dt);

        const float speed = glm::length(velocity.value);
        if (speed > stats.maxSpeed) {
            velocity.value *= stats.maxSpeed / speed;
        }
        transform.position += velocity.value * dt;
    }
}

} // namespace sc
