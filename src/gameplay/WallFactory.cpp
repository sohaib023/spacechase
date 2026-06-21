#include "gameplay/WallFactory.hpp"

#include "components/CollisionLayer.hpp"
#include "components/Health.hpp"
#include "components/Hitbox.hpp"
#include "components/PrevTransform.hpp"
#include "components/SpriteRender.hpp"
#include "components/StaticTag.hpp"
#include "components/Transform.hpp"
#include "components/Wall.hpp"

namespace sc {

Entity spawn_wall(Registry& registry, TextureHandle texture, TextureHandle normal,
                  glm::vec2 center, glm::vec2 halfExtents, bool destructible, float health) {
    const Entity entity = registry.create();

    registry.emplace<Transform>(entity, Transform{center, 0.0f});
    registry.emplace<PrevTransform>(entity, PrevTransform{center, 0.0f});

    SpriteRender sprite;
    sprite.texture = texture;
    sprite.normal = normal;
    sprite.size = halfExtents * 2.0f; // full extent in meters
    sprite.frameCount = 1;
    sprite.frame = 0;
    registry.emplace<SpriteRender>(entity, sprite);

    Hitbox hitbox;
    hitbox.shape = HitboxShape::AABB;
    hitbox.halfExtents = halfExtents;
    registry.emplace<Hitbox>(entity, hitbox);

    // Walls sit on the Wall layer; they don't initiate resolution (mask 0).
    registry.emplace<CollisionLayer>(entity, CollisionLayer{layer::Wall, 0u});
    registry.emplace<StaticTag>(entity);
    registry.emplace<Wall>(entity, Wall{destructible});
    if (destructible) {
        registry.emplace<Health>(entity, Health{health, health});
    }

    return entity;
}

} // namespace sc
