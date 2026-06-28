#pragma once

// Machine-gun beam (brief §6.3): a continuous hitscan. beam_step raycasts each
// active BeamRequest from its muzzle along facing vs layer::Wall, applies
// per-second damage to the hit target (queuing destruction at zero Health), and
// records a viz segment for the render section to draw. Single ray for now;
// cone-blocking (N discrete rays) is deferred.

#include "ecs/Registry.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sc {

struct Deferred;

struct BeamSegment {
    glm::vec2 start{0.0f, 0.0f};
    glm::vec2 end{0.0f, 0.0f};
    float width = 0.12f;
    glm::vec3 color{1.0f, 0.4f, 0.2f};
};

// Render-side list of beams to draw this frame (App-owned). beam_step clears and
// repopulates it; the latest fixed sub-step's beams are what gets drawn.
struct BeamVizList {
    std::vector<BeamSegment> beams;
};

void beam_step(Registry& registry, BeamVizList& viz, Deferred& deferred, float dt);

} // namespace sc
