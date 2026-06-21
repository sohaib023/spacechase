#include "gameplay/Ship.hpp"

#include <cmath>

namespace sc {

void ship_step(ShipState& state, const ShipConfig& cfg, const InputActions& input, float dt) {
    // Turn first so thrust applies along the updated facing.
    state.facing += input.turn * cfg.turnSpeed * dt;

    // facing 0 = +Y (up); rotating CCW gives direction (-sin, cos).
    const glm::vec2 forward{-std::sin(state.facing), std::cos(state.facing)};
    state.velocity += forward * (input.thrust * cfg.acceleration * dt);

    // Linear drag (momentum: velocity persists, bleeds off gradually).
    state.velocity -= state.velocity * (cfg.drag * dt);

    const float speed = glm::length(state.velocity);
    if (speed > cfg.maxSpeed) {
        state.velocity *= cfg.maxSpeed / speed;
    }

    state.position += state.velocity * dt;
}

ShipState ship_lerp(const ShipState& a, const ShipState& b, float t) {
    ShipState out;
    out.position = glm::mix(a.position, b.position, t);
    out.velocity = b.velocity;
    out.facing = a.facing + (b.facing - a.facing) * t; // small steps: plain lerp is fine
    return out;
}

} // namespace sc
