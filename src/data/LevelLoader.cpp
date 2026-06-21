#include "data/LevelLoader.hpp"

#include "core/Log.hpp"
#include "physics/Shapes.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <fstream>

namespace sc {

namespace {

glm::vec2 read_vec2(const nlohmann::json& j) {
    return glm::vec2(j.at(0).get<float>(), j.at(1).get<float>());
}

AABB read_rect(const nlohmann::json& j) {
    AABB box;
    box.center = read_vec2(j.at("center"));
    box.halfExtents = read_vec2(j.at("halfExtents"));
    return box;
}

} // namespace

bool load_level(const std::filesystem::path& path, LevelData& out) {
    std::ifstream stream(path);
    if (!stream) {
        SC_LOG_ERROR("Cannot open level file: %s", path.string().c_str());
        return false;
    }

    try {
        const nlohmann::json json = nlohmann::json::parse(stream);

        if (json.contains("playerSpawn")) {
            out.playerSpawn = read_vec2(json.at("playerSpawn"));
        }
        for (const nlohmann::json& rect : json.value("floor", nlohmann::json::array())) {
            out.level.floorRects.push_back(read_rect(rect));
        }
        for (const nlohmann::json& rect : json.value("voids", nlohmann::json::array())) {
            out.level.voidRects.push_back(read_rect(rect));
        }
        for (const nlohmann::json& wall : json.value("walls", nlohmann::json::array())) {
            WallInstance instance;
            instance.def = wall.at("def").get<std::string>();
            instance.center = read_vec2(wall.at("center"));
            if (wall.contains("halfExtents")) {
                instance.halfExtents = read_vec2(wall.at("halfExtents"));
                instance.hasHalfExtents = true;
            }
            out.walls.push_back(instance);
        }
    } catch (const std::exception& e) {
        SC_LOG_ERROR("Failed to parse level %s: %s", path.string().c_str(), e.what());
        return false;
    }

    return true;
}

} // namespace sc
