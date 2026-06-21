#pragma once

// Marks a wall entity. `destructible` walls carry Health and can be removed
// (debug-damaged in M3; real weapons in M5).

namespace sc {

struct Wall {
    bool destructible = false;
};

} // namespace sc
