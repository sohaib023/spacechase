#pragma once

// Typed registry of game-content definitions loaded from data files. M2 holds
// ship definitions; sibling maps (abilities/projectiles/...) slot in the same
// way later. This is the single source of truth the runtime spawns from and the
// editor will enumerate (brief §3.3, §7).

#include "data/ShipDef.hpp"
#include "data/WallDef.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace sc {

class DefinitionRegistry {
public:
    // Load every *.json in `dir` as a definition of the given kind. Bad files
    // are logged and skipped; returns false only if the dir is missing/empty.
    bool load_ships_from_dir(const std::filesystem::path& dir);
    bool load_walls_from_dir(const std::filesystem::path& dir);

    // Look up a definition by id; nullptr if absent.
    const ShipDef* find_ship(std::string_view id) const;
    const WallDef* find_wall(std::string_view id) const;

    const std::unordered_map<std::string, ShipDef>& ships() const { return m_ships; }
    const std::unordered_map<std::string, WallDef>& walls() const { return m_walls; }

private:
    std::unordered_map<std::string, ShipDef> m_ships;
    std::unordered_map<std::string, WallDef> m_walls;
};

} // namespace sc
