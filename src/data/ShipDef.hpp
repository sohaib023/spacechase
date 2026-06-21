#pragma once

// A ship definition: the data a ship entity is composed from. Loaded from JSON
// by the DefinitionRegistry and consumed by ShipFactory (and, later, the editor).
// Kept JSON-free so this header stays light; parsing lives in DefinitionRegistry.cpp.

#include <glm/glm.hpp>

#include <string>

namespace sc {

struct ShipStatsData {
    float acceleration = 0.0f;
    float maxSpeed = 0.0f;
    float turnSpeed = 0.0f;
    float drag = 0.0f;
};

struct ShipDef {
    std::string id;
    std::string displayName;
    std::string spriteSheet;       // path relative to SC_ASSET_DIR
    int frameCount = 1;            // horizontal frames in the sheet
    glm::vec2 frameSize{1.0f, 1.0f}; // meters
    float hitboxRadius = 0.5f;     // meters (circle collider)
    ShipStatsData stats;
};

} // namespace sc
