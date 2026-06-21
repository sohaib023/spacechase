#include "data/DefinitionRegistry.hpp"

#include "core/Log.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <fstream>
#include <utility>

namespace sc {

namespace {

glm::vec2 read_vec2(const nlohmann::json& j) {
    return glm::vec2(j.at(0).get<float>(), j.at(1).get<float>());
}

ShipDef parse_ship(const nlohmann::json& j) {
    ShipDef def;
    def.id = j.at("id").get<std::string>();
    def.displayName = j.value("displayName", def.id);

    const nlohmann::json& sprite = j.at("sprite");
    def.spriteSheet = sprite.at("sheet").get<std::string>();
    def.frameCount = sprite.value("frameCount", 1);
    def.frameSize = read_vec2(sprite.at("frameSize"));

    if (j.contains("hitbox")) {
        def.hitboxRadius = j.at("hitbox").value("radius", def.hitboxRadius);
    }

    const nlohmann::json& stats = j.at("stats");
    def.stats.acceleration = stats.at("acceleration").get<float>();
    def.stats.maxSpeed = stats.at("maxSpeed").get<float>();
    def.stats.turnSpeed = stats.at("turnSpeed").get<float>();
    def.stats.drag = stats.at("drag").get<float>();
    return def;
}

WallDef parse_wall(const nlohmann::json& j) {
    WallDef def;
    def.id = j.at("id").get<std::string>();
    def.sprite = j.at("sprite").get<std::string>();
    def.destructible = j.value("destructible", false);
    def.health = j.value("health", 0.0f);
    if (j.contains("halfExtents")) {
        def.halfExtents = read_vec2(j.at("halfExtents"));
    }
    return def;
}

// Load every *.json in `dir` into `target`, keyed by the parsed def's id.
// Bad files are logged and skipped; returns true if at least one loaded.
template <typename Def, typename ParseFn>
bool load_defs(const std::filesystem::path& dir, const char* kind,
               std::unordered_map<std::string, Def>& target, ParseFn parse) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        SC_LOG_ERROR("%s definitions directory not found: %s", kind, dir.string().c_str());
        return false;
    }

    int loaded = 0;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        std::ifstream stream(entry.path());
        if (!stream) {
            SC_LOG_ERROR("Cannot open definition file: %s", entry.path().string().c_str());
            continue;
        }
        try {
            Def def = parse(nlohmann::json::parse(stream));
            if (def.id.empty()) {
                SC_LOG_ERROR("%s definition missing 'id': %s", kind,
                             entry.path().string().c_str());
                continue;
            }
            SC_LOG_INFO("Loaded %s def '%s'", kind, def.id.c_str());
            target[def.id] = std::move(def);
            ++loaded;
        } catch (const std::exception& e) {
            SC_LOG_ERROR("Failed to parse %s: %s", entry.path().string().c_str(), e.what());
        }
    }
    return loaded > 0;
}

} // namespace

bool DefinitionRegistry::load_ships_from_dir(const std::filesystem::path& dir) {
    return load_defs<ShipDef>(dir, "Ship", m_ships, parse_ship);
}

bool DefinitionRegistry::load_walls_from_dir(const std::filesystem::path& dir) {
    return load_defs<WallDef>(dir, "Wall", m_walls, parse_wall);
}

const ShipDef* DefinitionRegistry::find_ship(std::string_view id) const {
    const auto it = m_ships.find(std::string(id));
    return it == m_ships.end() ? nullptr : &it->second;
}

const WallDef* DefinitionRegistry::find_wall(std::string_view id) const {
    const auto it = m_walls.find(std::string(id));
    return it == m_walls.end() ? nullptr : &it->second;
}

} // namespace sc
