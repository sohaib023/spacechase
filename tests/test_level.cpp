#include <doctest/doctest.h>

#include "data/DefinitionRegistry.hpp"
#include "data/LevelLoader.hpp"

#include <string>

using namespace sc;

TEST_CASE("level JSON loads floor, voids, walls, and spawn") {
    LevelData data;
    REQUIRE(load_level(std::string(SC_ASSET_DIR) + "/levels/test_m3.json", data));

    CHECK(data.playerSpawn.x == doctest::Approx(0.0f));
    CHECK(data.playerSpawn.y == doctest::Approx(0.0f));
    CHECK(data.level.floorRects.size() == 4);
    CHECK(data.walls.size() == 5);

    CHECK(data.level.has_floor({0.0f, 0.0f}));        // on the floor
    CHECK_FALSE(data.level.has_floor({5.0f, -4.0f}));  // in the pit (floor gap)
    CHECK_FALSE(data.level.has_floor({100.0f, 0.0f})); // off the floor
}

TEST_CASE("wall definitions load with destructibility + health") {
    DefinitionRegistry defs;
    REQUIRE(defs.load_walls_from_dir(std::string(SC_ASSET_DIR) + "/definitions/walls"));

    const WallDef* crate = defs.find_wall("crate");
    REQUIRE(crate != nullptr);
    CHECK(crate->destructible);
    CHECK(crate->health == doctest::Approx(100.0f));
    CHECK(crate->surface == SurfaceType::Absorbent);

    const WallDef* wall = defs.find_wall("wall");
    REQUIRE(wall != nullptr);
    CHECK_FALSE(wall->destructible);
    CHECK(wall->surface == SurfaceType::Reflective);

    CHECK(defs.find_wall("nonexistent") == nullptr);
}
