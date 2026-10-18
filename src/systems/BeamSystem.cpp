#include "systems/BeamSystem.hpp"

#include "components/BeamRequest.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Health.hpp"
#include "components/SurfaceResponse.hpp"
#include "components/Transform.hpp"
#include "gameplay/Spawning.hpp"
#include "physics/Raycast.hpp"

#include <cmath>

namespace sc {

namespace {
// Muzzle offset ahead of the hull so the beam starts clear of the firing ship.
constexpr float kBeamMuzzle = 1.0f;
constexpr int kMaxBeamSegments = 16; // hard cap for unlimited bounces in tight spaces
constexpr float kSurfaceEps = 1e-3f; // re-launch offset off the surface after a bounce
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
        glm::vec2 dir{-std::sin(transform.rotation), std::cos(transform.rotation)};
        glm::vec2 origin = transform.position + dir * kBeamMuzzle;
        float remaining = beam.maxDist; // the beam's range, shared by all segments
        int bouncesLeft = beam.bounce ? beam.maxBounces : 0;

        // Trace the beam as a polyline: each segment ends at a hit (damage) and
        // continues only off a Reflective surface while bounces and range remain.
        for (int segment = 0; segment < kMaxBeamSegments && remaining > 1e-4f; ++segment) {
            const RayHit hit = raycast(registry, origin, dir, remaining, layer::Wall);
            const float length = hit.hit ? hit.t : remaining;
            viz.beams.push_back(
                BeamSegment{origin, origin + dir * length, beam.width, beam.color});
            if (!hit.hit) {
                break;
            }

            if (Health* health = registry.try_get<Health>(hit.entity)) {
                health->current -= beam.damage * dt; // per-second damage
                if (health->current <= 0.0f) {
                    deferred.despawns.push_back(hit.entity);
                }
            }

            const SurfaceResponse* surface = registry.try_get<SurfaceResponse>(hit.entity);
            const bool reflective =
                surface != nullptr && surface->type == SurfaceType::Reflective;
            const bool validNormal = glm::dot(hit.normal, hit.normal) > 0.5f;
            if (bouncesLeft == 0 || !reflective || !validNormal) {
                break;
            }
            if (bouncesLeft > 0) {
                --bouncesLeft;
            }
            dir -= 2.0f * glm::dot(dir, hit.normal) * hit.normal;
            origin = hit.point + hit.normal * kSurfaceEps;
            remaining -= hit.t;
        }
    }
}

} // namespace sc
