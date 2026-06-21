#pragma once

// Uniform-grid broadphase over continuous meter space: bucket collider AABBs
// into cells so queries only test nearby entities, not all N. Rebuilt each
// fixed step. `cellSize` is tunable (low entity counts in M3; real tuning M5).

#include "ecs/Registry.hpp"
#include "physics/Shapes.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace sc {

class SpatialHash {
public:
    explicit SpatialHash(float cellSize = 4.0f) : m_cellSize(cellSize) {}

    void clear();
    void insert(Entity entity, const AABB& bounds);

    // Append entities whose cells overlap `bounds` into `out`, sorted by id and
    // de-duplicated (stable order → deterministic resolution, brief §10.4).
    void query(const AABB& bounds, std::vector<Entity>& out) const;

private:
    std::int64_t cell_key(int cx, int cy) const;

    float m_cellSize;
    std::unordered_map<std::int64_t, std::vector<Entity>> m_cells;
};

} // namespace sc
