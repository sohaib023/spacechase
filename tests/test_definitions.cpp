#include <doctest/doctest.h>

#include "data/DefinitionRegistry.hpp"

#include <string>

// Loads the real shipped definitions (SC_ASSET_DIR) and checks the schema +
// values parse, and that lookups behave.
TEST_CASE("ship definitions load from JSON") {
    sc::DefinitionRegistry defs;
    const bool ok = defs.load_ships_from_dir(std::string(SC_ASSET_DIR) + "/definitions/ships");
    CHECK(ok);

    const sc::ShipDef* player = defs.find_ship("player_ship");
    REQUIRE(player != nullptr);
    CHECK(player->id == "player_ship");
    CHECK(player->spriteSheet == "sprites/ship_sheet.png");
    CHECK(player->frameCount == 3);
    CHECK(player->stats.acceleration == doctest::Approx(40.0f));
    CHECK(player->stats.maxSpeed == doctest::Approx(20.0f));
    CHECK(player->stats.turnSpeed == doctest::Approx(3.2f));
    CHECK(player->stats.drag == doctest::Approx(1.2f));

    CHECK(defs.find_ship("does_not_exist") == nullptr);
}

TEST_CASE("missing definitions directory fails gracefully") {
    sc::DefinitionRegistry defs;
    CHECK_FALSE(defs.load_ships_from_dir("definitely/not/a/real/path"));
    CHECK(defs.ships().empty());
}
