#include "data/DefinitionRegistry.hpp"

#include "core/Log.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <fstream>

namespace sc {

namespace {

// Parse one ship definition object. Throws on missing/wrong-typed required
// fields (nlohmann::json::at), which the caller catches per-file.
ShipDef parse_ship(const nlohmann::json& j) {
    ShipDef def;
    def.id = j.at("id").get<std::string>();
    def.displayName = j.value("displayName", def.id);

    const nlohmann::json& sprite = j.at("sprite");
    def.spriteSheet = sprite.at("sheet").get<std::string>();
    def.frameCount = sprite.value("frameCount", 1);
    const nlohmann::json& frameSize = sprite.at("frameSize");
    def.frameSize = glm::vec2(frameSize.at(0).get<float>(), frameSize.at(1).get<float>());

    const nlohmann::json& stats = j.at("stats");
    def.stats.acceleration = stats.at("acceleration").get<float>();
    def.stats.maxSpeed = stats.at("maxSpeed").get<float>();
    def.stats.turnSpeed = stats.at("turnSpeed").get<float>();
    def.stats.drag = stats.at("drag").get<float>();
    return def;
}

} // namespace

bool DefinitionRegistry::load_ships_from_dir(const std::filesystem::path& dir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        SC_LOG_ERROR("Ship definitions directory not found: %s", dir.string().c_str());
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
            const nlohmann::json json = nlohmann::json::parse(stream);
            ShipDef def = parse_ship(json);
            if (def.id.empty()) {
                SC_LOG_ERROR("Ship definition missing 'id': %s",
                             entry.path().string().c_str());
                continue;
            }
            SC_LOG_INFO("Loaded ship def '%s' (accel=%.0f maxSpeed=%.0f turn=%.2f drag=%.2f)",
                        def.id.c_str(), def.stats.acceleration, def.stats.maxSpeed,
                        def.stats.turnSpeed, def.stats.drag);
            m_ships[def.id] = std::move(def);
            ++loaded;
        } catch (const std::exception& e) {
            SC_LOG_ERROR("Failed to parse %s: %s", entry.path().string().c_str(), e.what());
        }
    }

    return loaded > 0;
}

const ShipDef* DefinitionRegistry::find_ship(std::string_view id) const {
    const auto it = m_ships.find(std::string(id));
    return it == m_ships.end() ? nullptr : &it->second;
}

} // namespace sc
