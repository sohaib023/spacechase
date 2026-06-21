#include "physics/SpatialHash.hpp"

#include <algorithm>
#include <cmath>

namespace sc {

std::int64_t SpatialHash::cell_key(int cx, int cy) const {
    return (static_cast<std::int64_t>(cx) << 32) ^
           static_cast<std::int64_t>(static_cast<std::uint32_t>(cy));
}

void SpatialHash::clear() {
    m_cells.clear();
}

void SpatialHash::insert(Entity entity, const AABB& bounds) {
    const glm::vec2 lo = bounds.min();
    const glm::vec2 hi = bounds.max();
    const int x0 = static_cast<int>(std::floor(lo.x / m_cellSize));
    const int x1 = static_cast<int>(std::floor(hi.x / m_cellSize));
    const int y0 = static_cast<int>(std::floor(lo.y / m_cellSize));
    const int y1 = static_cast<int>(std::floor(hi.y / m_cellSize));
    for (int cy = y0; cy <= y1; ++cy) {
        for (int cx = x0; cx <= x1; ++cx) {
            m_cells[cell_key(cx, cy)].push_back(entity);
        }
    }
}

void SpatialHash::query(const AABB& bounds, std::vector<Entity>& out) const {
    const glm::vec2 lo = bounds.min();
    const glm::vec2 hi = bounds.max();
    const int x0 = static_cast<int>(std::floor(lo.x / m_cellSize));
    const int x1 = static_cast<int>(std::floor(hi.x / m_cellSize));
    const int y0 = static_cast<int>(std::floor(lo.y / m_cellSize));
    const int y1 = static_cast<int>(std::floor(hi.y / m_cellSize));
    for (int cy = y0; cy <= y1; ++cy) {
        for (int cx = x0; cx <= x1; ++cx) {
            const auto it = m_cells.find(cell_key(cx, cy));
            if (it != m_cells.end()) {
                out.insert(out.end(), it->second.begin(), it->second.end());
            }
        }
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
}

} // namespace sc
