#include "physics/Raycast.hpp"

#include "components/CollisionLayer.hpp"
#include "components/Hitbox.hpp"
#include "components/Transform.hpp"

#include <algorithm>
#include <cmath>

namespace sc {

bool ray_vs_aabb(glm::vec2 origin, glm::vec2 dir, const AABB& box, float maxDist,
                 float& tOut, glm::vec2& normalOut) {
    const glm::vec2 lo = box.min();
    const glm::vec2 hi = box.max();
    float tmin = 0.0f;
    float tmax = maxDist;
    glm::vec2 normal{0.0f, 0.0f};

    for (int axis = 0; axis < 2; ++axis) {
        if (std::abs(dir[axis]) < 1e-8f) {
            if (origin[axis] < lo[axis] || origin[axis] > hi[axis]) {
                return false; // parallel and outside the slab
            }
            continue;
        }
        const float inv = 1.0f / dir[axis];
        float t1 = (lo[axis] - origin[axis]) * inv;
        float t2 = (hi[axis] - origin[axis]) * inv;
        float sign = -1.0f; // entering from the min side → outward normal -1 on this axis
        if (t1 > t2) {
            std::swap(t1, t2);
            sign = 1.0f;
        }
        if (t1 > tmin) {
            tmin = t1;
            normal = glm::vec2(0.0f);
            normal[axis] = sign;
        }
        tmax = std::min(tmax, t2);
        if (tmin > tmax) {
            return false;
        }
    }

    tOut = tmin;
    normalOut = normal;
    return true;
}

bool ray_vs_circle(glm::vec2 origin, glm::vec2 dir, const Circle& circle, float maxDist,
                   float& tOut, glm::vec2& normalOut) {
    const glm::vec2 m = origin - circle.center;
    const float b = glm::dot(m, dir);
    const float c = glm::dot(m, m) - circle.radius * circle.radius;
    if (c > 0.0f && b > 0.0f) {
        return false; // origin outside, pointing away
    }
    const float disc = b * b - c;
    if (disc < 0.0f) {
        return false;
    }
    float t = -b - std::sqrt(disc);
    if (t < 0.0f) {
        t = 0.0f; // origin inside the circle
    }
    if (t > maxDist) {
        return false;
    }
    tOut = t;
    const glm::vec2 point = origin + dir * t;
    const glm::vec2 d = point - circle.center;
    const float len = std::sqrt(glm::dot(d, d));
    normalOut = len > 1e-6f ? d / len : glm::vec2(0.0f, 1.0f);
    return true;
}

RayHit raycast(const Registry& registry, glm::vec2 origin, glm::vec2 dir, float maxDist,
               std::uint32_t mask) {
    RayHit best;
    const float dirLen = std::sqrt(glm::dot(dir, dir));
    if (dirLen < 1e-8f) {
        return best;
    }
    const glm::vec2 d = dir / dirLen;
    float nearest = maxDist;

    auto view = registry.view<const Transform, const Hitbox, const CollisionLayer>();
    for (const Entity entity : view) {
        if ((mask & view.get<const CollisionLayer>(entity).layer) == 0) {
            continue;
        }
        const Transform& transform = view.get<const Transform>(entity);
        const Hitbox& hitbox = view.get<const Hitbox>(entity);
        const glm::vec2 center = transform.position + hitbox.offset;

        float t = 0.0f;
        glm::vec2 normal{0.0f, 0.0f};
        const bool hit =
            hitbox.shape == HitboxShape::AABB
                ? ray_vs_aabb(origin, d, AABB{center, hitbox.halfExtents}, nearest, t, normal)
                : ray_vs_circle(origin, d, Circle{center, hitbox.radius}, nearest, t, normal);
        if (hit && t <= nearest) {
            nearest = t;
            best.hit = true;
            best.t = t;
            best.point = origin + d * t;
            best.normal = normal;
            best.entity = entity;
        }
    }
    return best;
}

} // namespace sc
