#include "systems/InputSystem.hpp"

#include "components/PlayerControlled.hpp"
#include "components/ShipControl.hpp"

namespace sc {

void input_apply(Registry& registry, const InputActions& actions) {
    auto view = registry.view<ShipControl, PlayerControlled>();
    for (const Entity entity : view) {
        ShipControl& control = view.get<ShipControl>(entity);
        control.thrust = actions.thrust;
        control.turn = actions.turn;
    }
}

} // namespace sc
