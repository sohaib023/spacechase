#include "systems/ProjectileMoveSystem.hpp"

#include "components/Projectile.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"

namespace sc {

void projectile_move_step(Registry& registry, float dt) {
    auto view = registry.view<Transform, Velocity, Projectile>();
    for (const Entity entity : view) {
        view.get<Transform>(entity).position += view.get<Velocity>(entity).value * dt;
    }
}

} // namespace sc
