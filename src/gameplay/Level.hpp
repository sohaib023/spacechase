#pragma once

// The level's floor in continuous meters: a union of floor rectangles with
// optional void rectangles (holes) cut out of them. A point has floor if it is
// inside some floor rect and not inside any void rect. (Single layer for M3.)

#include "physics/Shapes.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sc {

struct Level {
    std::vector<AABB> floorRects;
    std::vector<AABB> voidRects;

    bool has_floor(glm::vec2 point) const {
        bool onFloor = false;
        for (const AABB& rect : floorRects) {
            if (inside(rect, point)) {
                onFloor = true;
                break;
            }
        }
        if (!onFloor) {
            return false;
        }
        for (const AABB& hole : voidRects) {
            if (inside(hole, point)) {
                return false;
            }
        }
        return true;
    }

private:
    static bool inside(const AABB& box, glm::vec2 p) {
        const glm::vec2 lo = box.min();
        const glm::vec2 hi = box.max();
        return p.x >= lo.x && p.x <= hi.x && p.y >= lo.y && p.y <= hi.y;
    }
};

} // namespace sc
