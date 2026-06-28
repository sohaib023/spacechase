#include "systems/ForceFieldSystem.hpp"

#include "components/Deflector.hpp"
#include "components/ForceField.hpp"
#include "components/ForceResponse.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "physics/ForceApi.hpp"

#include <algorithm>
#include <cmath>

namespace sc {

namespace {

float falloff_factor(FalloffType falloff, float dist, float range) {
    switch (falloff) {
    case FalloffType::None:
        return 1.0f;
    case FalloffType::Linear:
        return std::max(0.0f, 1.0f - dist / range);
    case FalloffType::InverseSquare:
        return 1.0f / (1.0f + dist * dist);
    }
    return 1.0f;
}

// Radial push from `source` outward to `targetPos` (used by ForceField radial +
// active Deflector ships). strength > 0 pushes away.
void apply_radial(Velocity& velocity, glm::vec2 targetPos, glm::vec2 source, float strength,
                  float range, FalloffType falloff, float dt) {
    const glm::vec2 delta = targetPos - source;
    const float dist = std::sqrt(glm::dot(delta, delta));
    if (dist > range || dist < 1e-4f) {
        return;
    }
    const glm::vec2 dir = delta / dist;
    apply_impulse(velocity.value, dir * (strength * falloff_factor(falloff, dist, range) * dt));
}

} // namespace

void forcefield_step(Registry& registry, float dt) {
    auto targets = registry.view<Transform, Velocity, ForceResponse>();

    // Explicit ForceField sources (level wind zones, etc.).
    auto fields = registry.view<Transform, ForceField>();
    for (const Entity source : fields) {
        const glm::vec2 sourcePos = fields.get<Transform>(source).position;
        const ForceField& field = fields.get<ForceField>(source);
        for (const Entity target : targets) {
            if (target == source ||
                targets.get<ForceResponse>(target).type != ForceResponseType::Deflectable) {
                continue;
            }
            Velocity& velocity = targets.get<Velocity>(target);
            const glm::vec2 targetPos = targets.get<Transform>(target).position;
            if (field.shape == FieldShape::Radial) {
                apply_radial(velocity, targetPos, sourcePos, field.strength, field.range,
                             field.falloff, dt);
            } else { // Directional: constant push along dir within range of the source.
                const glm::vec2 delta = targetPos - sourcePos;
                const float dist = std::sqrt(glm::dot(delta, delta));
                if (dist <= field.range) {
                    apply_impulse(velocity.value,
                                  field.dir *
                                      (field.strength * falloff_factor(field.falloff, dist,
                                                                       field.range) * dt));
                }
            }
        }
    }

    // Active deflector ships act as radial repulsion sources (the defensive ability).
    auto deflectors = registry.view<Transform, Deflector>();
    for (const Entity source : deflectors) {
        const Deflector& deflector = deflectors.get<Deflector>(source);
        if (!deflector.active) {
            continue;
        }
        const glm::vec2 sourcePos = deflectors.get<Transform>(source).position;
        for (const Entity target : targets) {
            if (target == source ||
                targets.get<ForceResponse>(target).type != ForceResponseType::Deflectable) {
                continue;
            }
            apply_radial(targets.get<Velocity>(target), targets.get<Transform>(target).position,
                         sourcePos, deflector.strength, deflector.range, FalloffType::Linear, dt);
        }
    }
}

} // namespace sc
