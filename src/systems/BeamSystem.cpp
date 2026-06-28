#include "systems/BeamSystem.hpp"

#include "components/BeamRequest.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Health.hpp"
#include "components/Transform.hpp"
#include "gameplay/Spawning.hpp"
#include "physics/Raycast.hpp"

#include <cmath>

namespace sc {

namespace {
// Muzzle offset ahead of the hull so the beam starts clear of the firing ship.
constexpr float kBeamMuzzle = 1.0f;
} // namespace

void beam_step(Registry& registry, BeamVizList& viz, Deferred& deferred, float dt) {
    viz.beams.clear();

    auto view = registry.view<Transform, BeamRequest>();
    for (const Entity shooter : view) {
        const BeamRequest& beam = view.get<BeamRequest>(shooter);
        if (!beam.active) {
            continue;
        }
        const Transform& transform = view.get<Transform>(shooter);
        const glm::vec2 dir{-std::sin(transform.rotation), std::cos(transform.rotation)};
        const glm::vec2 origin = transform.position + dir * kBeamMuzzle;

        const RayHit hit = raycast(registry, origin, dir, beam.maxDist, layer::Wall);
        const float length = hit.hit ? hit.t : beam.maxDist;

        if (hit.hit) {
            if (Health* health = registry.try_get<Health>(hit.entity)) {
                health->current -= beam.damage * dt; // per-second damage
                if (health->current <= 0.0f) {
                    deferred.despawns.push_back(hit.entity);
                }
            }
        }

        viz.beams.push_back(BeamSegment{origin, origin + dir * length, beam.width, beam.color});
    }
}

} // namespace sc
