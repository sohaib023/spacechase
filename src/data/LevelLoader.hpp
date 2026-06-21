#pragma once

// Loads a level JSON into LevelData (pure data; no GPU). Returns false (logged)
// if the file is missing or malformed.

#include "data/LevelDef.hpp"

#include <filesystem>

namespace sc {

bool load_level(const std::filesystem::path& path, LevelData& out);

} // namespace sc
