#include <doctest/doctest.h>

#include "components/Light.hpp"
#include "components/Transform.hpp"
#include "ecs/Registry.hpp"
#include "systems/LightingSystem.hpp"

#include <glm/glm.hpp>

using namespace sc;

TEST_CASE("collect_lights packs a point light into the UBO layout") {
    Registry reg;
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{{3.0f, 4.0f}, 0.0f});
    Light l;
    l.type = LightType::Point;
    l.color = {1.0f, 0.5f, 0.25f};
    l.intensity = 2.0f;
    l.height = 1.5f;
    l.range = 10.0f;
    reg.emplace<Light>(e, l);

    const LightsUBO ubo =
        collect_lights(reg, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), glm::vec4(16.0f, 0.5f, 0.0f, 0.0f));

    CHECK(ubo.counts.x == 1);
    CHECK(ubo.lights[0].posHeight.x == doctest::Approx(3.0f)); // world x
    CHECK(ubo.lights[0].posHeight.y == doctest::Approx(4.0f)); // world y
    CHECK(ubo.lights[0].posHeight.z == doctest::Approx(1.5f)); // height
    CHECK(ubo.lights[0].posHeight.w == doctest::Approx(0.0f)); // type = point
    CHECK(ubo.lights[0].color.a == doctest::Approx(2.0f));     // intensity
    CHECK(ubo.lights[0].rangeDir.x == doctest::Approx(10.0f)); // range
    CHECK(ubo.ambient.w == doctest::Approx(1.0f));
    CHECK(ubo.params.x == doctest::Approx(16.0f));
}

TEST_CASE("collect_lights marks directional lights") {
    Registry reg;
    const Entity e = reg.create();
    reg.emplace<Transform>(e, Transform{{0.0f, 0.0f}, 0.0f});
    Light l;
    l.type = LightType::Directional;
    l.direction = {0.3f, -1.0f};
    reg.emplace<Light>(e, l);

    const LightsUBO ubo = collect_lights(reg, glm::vec4(0.0f), glm::vec4(0.0f));
    CHECK(ubo.lights[0].posHeight.w == doctest::Approx(1.0f)); // type = directional
    CHECK(ubo.lights[0].rangeDir.y == doctest::Approx(0.3f));
    CHECK(ubo.lights[0].rangeDir.z == doctest::Approx(-1.0f));
}

TEST_CASE("collect_lights clamps to kMaxLights") {
    Registry reg;
    for (int i = 0; i < kMaxLights + 5; ++i) {
        const Entity e = reg.create();
        reg.emplace<Transform>(e, Transform{{0.0f, 0.0f}, 0.0f});
        reg.emplace<Light>(e, Light{});
    }
    const LightsUBO ubo = collect_lights(reg, glm::vec4(0.0f), glm::vec4(0.0f));
    CHECK(ubo.counts.x == kMaxLights);
}
