#include <doctest/doctest.h>

#include "physics/SpatialHash.hpp"

#include <algorithm>
#include <vector>

using namespace sc;

namespace {
AABB box_at(float x, float y, float half = 0.5f) {
    return AABB{{x, y}, {half, half}};
}
} // namespace

TEST_CASE("spatial hash returns nearby entities and excludes distant ones") {
    SpatialHash hash(4.0f);
    const Entity a = static_cast<Entity>(1);
    const Entity b = static_cast<Entity>(2);
    const Entity far = static_cast<Entity>(3);
    hash.insert(a, box_at(0.0f, 0.0f));
    hash.insert(b, box_at(1.0f, 0.0f));
    hash.insert(far, box_at(100.0f, 100.0f));

    std::vector<Entity> out;
    hash.query(box_at(0.5f, 0.0f, 1.0f), out);

    CHECK(std::find(out.begin(), out.end(), a) != out.end());
    CHECK(std::find(out.begin(), out.end(), b) != out.end());
    CHECK(std::find(out.begin(), out.end(), far) == out.end());
}

TEST_CASE("spatial hash query is sorted and de-duplicated") {
    SpatialHash hash(1.0f);
    // A big box spanning many cells → the same entity lands in multiple cells.
    const Entity e = static_cast<Entity>(7);
    hash.insert(e, AABB{{0.0f, 0.0f}, {5.0f, 5.0f}});

    std::vector<Entity> out;
    hash.query(AABB{{0.0f, 0.0f}, {5.0f, 5.0f}}, out);
    CHECK(out.size() == 1); // de-duplicated despite spanning many cells

    SpatialHash h2(2.0f);
    h2.insert(static_cast<Entity>(5), box_at(0, 0));
    h2.insert(static_cast<Entity>(2), box_at(0, 0));
    h2.insert(static_cast<Entity>(9), box_at(0, 0));
    out.clear();
    h2.query(box_at(0, 0, 1.0f), out);
    CHECK(std::is_sorted(out.begin(), out.end()));
}

TEST_CASE("clear empties the hash") {
    SpatialHash hash(4.0f);
    hash.insert(static_cast<Entity>(1), box_at(0, 0));
    hash.clear();
    std::vector<Entity> out;
    hash.query(box_at(0, 0, 1.0f), out);
    CHECK(out.empty());
}
