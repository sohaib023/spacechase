#pragma once

// Elastic camera-follow strategies. Translation only (the camera never rotates).
// Polymorphic so models are swappable and several can coexist; App holds one.

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace sc {

class CameraFollow {
public:
    virtual ~CameraFollow() = default;
    // Move `current` toward `target` over `dt` seconds; return the new position.
    virtual glm::vec2 update(glm::vec2 current, glm::vec2 target, float dt) = 0;
    virtual const char* name() const = 0;
};

// Critically-damped spring ("SmoothDamp"): eases in, lags while moving, settles
// exactly on the target with no overshoot. Frame-rate independent. One knob.
class SmoothDampFollow : public CameraFollow {
public:
    float smoothTime = 0.25f; // ~seconds to converge

    glm::vec2 update(glm::vec2 current, glm::vec2 target, float dt) override {
        const float st = std::max(smoothTime, 1e-4f);
        const float omega = 2.0f / st;
        const float x = omega * dt;
        const float decay = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
        const glm::vec2 change = current - target;
        const glm::vec2 temp = (m_velocity + omega * change) * dt;
        m_velocity = (m_velocity - omega * temp) * decay;
        return target + (change + temp) * decay;
    }
    const char* name() const override { return "SmoothDamp (critically damped)"; }

private:
    glm::vec2 m_velocity{0.0f, 0.0f};
};

// Exponential lag: move a frame-rate-corrected fraction toward the target each
// frame. Smooth trailing lag, no overshoot, stateless. One knob.
class ExponentialFollow : public CameraFollow {
public:
    float responsiveness = 6.0f; // higher = snappier

    glm::vec2 update(glm::vec2 current, glm::vec2 target, float dt) override {
        const float t = 1.0f - std::exp(-std::max(responsiveness, 0.0f) * dt);
        return current + (target - current) * t;
    }
    const char* name() const override { return "Exponential lag"; }
};

} // namespace sc
