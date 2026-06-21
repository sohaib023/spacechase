#pragma once

// Hit points. On destructible walls now (debug-damaged in M3); ships and other
// destructibles use it once weapons exist (M5).

namespace sc {

struct Health {
    float current = 1.0f;
    float max = 1.0f;
};

} // namespace sc
