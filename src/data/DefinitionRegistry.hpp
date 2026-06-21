#pragma once

// Typed registry of game-content definitions loaded from data files. M2 holds
// ship definitions; sibling maps (abilities/projectiles/...) slot in the same
// way later. This is the single source of truth the runtime spawns from and the
// editor will enumerate (brief §3.3, §7).

#include "data/ShipDef.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace sc {

class DefinitionRegistry {
public:
    // Load every *.json in `dir` as a ship definition. Bad files are logged and
    // skipped; returns false only if the directory is missing or yields nothing.
    bool load_ships_from_dir(const std::filesystem::path& dir);

    // Look up a ship definition by id; nullptr if absent.
    const ShipDef* find_ship(std::string_view id) const;

    const std::unordered_map<std::string, ShipDef>& ships() const { return m_ships; }

private:
    std::unordered_map<std::string, ShipDef> m_ships;
};

} // namespace sc
