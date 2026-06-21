#include "gameplay/ShipFactory.hpp"

#include "components/Altitude.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Hitbox.hpp"
#include "components/PlayerControlled.hpp"
#include "components/PrevTransform.hpp"
#include "components/ShipControl.hpp"
#include "components/ShipStats.hpp"
#include "components/SpriteRender.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "data/ShipDef.hpp"

namespace sc {

Entity spawn_ship(Registry& registry, const ShipDef& def, TextureHandle texture,
                  glm::vec2 position, float facing, bool playerControlled) {
    const Entity entity = registry.create();

    // Transform and PrevTransform start equal so the first frame's interpolation
    // is a no-op (matches M1, where m_shipPrev == m_ship initially).
    registry.emplace<Transform>(entity, Transform{position, facing});
    registry.emplace<PrevTransform>(entity, PrevTransform{position, facing});
    registry.emplace<Velocity>(entity);
    registry.emplace<ShipControl>(entity);
    registry.emplace<ShipStats>(entity, ShipStats{def.stats.acceleration, def.stats.maxSpeed,
                                                  def.stats.turnSpeed, def.stats.drag});

    SpriteRender sprite;
    sprite.texture = texture;
    sprite.size = def.frameSize;
    sprite.frameCount = def.frameCount;
    sprite.frame = def.frameCount > 1 ? def.frameCount / 2 : 0;
    registry.emplace<SpriteRender>(entity, sprite);

    Hitbox hitbox;
    hitbox.shape = HitboxShape::Circle;
    hitbox.radius = def.hitboxRadius;
    registry.emplace<Hitbox>(entity, hitbox);
    // Ships collide with walls.
    registry.emplace<CollisionLayer>(entity, CollisionLayer{layer::Ship, layer::Wall});
    registry.emplace<Altitude>(entity); // hovering at default height

    if (playerControlled) {
        registry.emplace<PlayerControlled>(entity);
    }
    return entity;
}

} // namespace sc
