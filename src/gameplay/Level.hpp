#pragma once

// The level's floor in continuous meters: a union of floor rectangles. Gaps
// between them are void (off into infinity) — the void background shows through.
// A point has floor if it is inside some floor rect. (Single layer for M3.)

#include "physics/Shapes.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sc {

struct Level {
    std::vector<AABB> floorRects;

    bool has_floor(glm::vec2 point) const {
        for (const AABB& rect : floorRects) {
            if (inside(rect, point)) {
                return true;
            }
        }
        return false;
    }

private:
    static bool inside(const AABB& box, glm::vec2 p) {
        const glm::vec2 lo = box.min();
        const glm::vec2 hi = box.max();
        return p.x >= lo.x && p.x <= hi.x && p.y >= lo.y && p.y <= hi.y;
    }
};

} // namespace sc
