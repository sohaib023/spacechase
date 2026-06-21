#include "systems/LightingSystem.hpp"

#include "components/Light.hpp"
#include "components/Transform.hpp"

namespace sc {

LightsUBO collect_lights(const Registry& registry, glm::vec4 ambient, glm::vec4 params) {
    LightsUBO ubo;
    ubo.ambient = ambient;
    ubo.params = params;

    int count = 0;
    auto view = registry.view<const Transform, const Light>();
    for (const Entity entity : view) {
        if (count >= kMaxLights) {
            break;
        }
        const Transform& transform = view.get<const Transform>(entity);
        const Light& light = view.get<const Light>(entity);

        GpuLight& g = ubo.lights[count];
        g.posHeight = glm::vec4(transform.position, light.height,
                                light.type == LightType::Directional ? 1.0f : 0.0f);
        g.color = glm::vec4(light.color, light.intensity);
        g.rangeDir = glm::vec4(light.range, light.direction.x, light.direction.y, 0.0f);
        ++count;
    }
    ubo.counts = glm::ivec4(count, 0, 0, 0);
    return ubo;
}

} // namespace sc
