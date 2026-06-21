#include "systems/RenderCollectSystem.hpp"

#include "components/PrevTransform.hpp"
#include "components/ShipControl.hpp"
#include "components/SpriteRender.hpp"
#include "components/Transform.hpp"
#include "render/Renderer.hpp"

namespace sc {

void render_collect(Registry& registry, Renderer& renderer, float alpha) {
    auto view = registry.view<PrevTransform, Transform, SpriteRender>();
    for (const Entity entity : view) {
        const PrevTransform& prev = view.get<PrevTransform>(entity);
        const Transform& current = view.get<Transform>(entity);
        SpriteRender& sprite = view.get<SpriteRender>(entity);
        if (sprite.texture == nullptr || sprite.frameCount < 1) {
            continue;
        }

        // Plain lerp (matches M1's ship_lerp — NOT shortest-arc on rotation).
        const glm::vec2 position = glm::mix(prev.position, current.position, alpha);
        const float rotation = prev.rotation + (current.rotation - prev.rotation) * alpha;

        // Tilt frame from control intent (if any): 3-frame sheet = left/neutral/right.
        int frame = sprite.frameCount > 1 ? sprite.frameCount / 2 : 0;
        if (sprite.frameCount >= 3) {
            if (const ShipControl* control = registry.try_get<ShipControl>(entity)) {
                if (control->turn > 0.2f) {
                    frame = 0;
                } else if (control->turn < -0.2f) {
                    frame = 2;
                } else {
                    frame = 1;
                }
            }
        }
        sprite.frame = frame;

        const float frameW = 1.0f / static_cast<float>(sprite.frameCount);
        const glm::vec2 uvMin{static_cast<float>(frame) * frameW, 0.0f};
        const glm::vec2 uvMax{static_cast<float>(frame + 1) * frameW, 1.0f};
        renderer.draw(*sprite.texture, position, sprite.size, rotation, uvMin, uvMax);
    }
}

} // namespace sc
