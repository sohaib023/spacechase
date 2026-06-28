#include "gameplay/ProjectileFactory.hpp"

#include "components/CollisionLayer.hpp"
#include "components/Damage.hpp"
#include "components/ForceResponse.hpp"
#include "components/Hitbox.hpp"
#include "components/Lifetime.hpp"
#include "components/Owner.hpp"
#include "components/PrevTransform.hpp"
#include "components/Projectile.hpp"
#include "components/SpriteRender.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "gameplay/Spawning.hpp"

namespace sc {

Entity spawn_projectile(Registry& registry, const ProjectileSpawn& spawn) {
    const Entity entity = registry.create();

    registry.emplace<Transform>(entity, Transform{spawn.position, spawn.rotation});
    registry.emplace<PrevTransform>(entity, PrevTransform{spawn.position, spawn.rotation});
    registry.emplace<Velocity>(entity, Velocity{spawn.velocity});
    registry.emplace<Projectile>(entity);
    registry.emplace<Lifetime>(entity, Lifetime{spawn.params.lifetime});
    registry.emplace<Damage>(entity, Damage{spawn.params.damage});
    registry.emplace<Owner>(entity, Owner{spawn.owner, spawn.team});

    SpriteRender sprite;
    sprite.texture = spawn.albedo;
    sprite.normal = spawn.normal;
    sprite.size = spawn.params.spriteSize;
    sprite.frameCount = 1;
    sprite.frame = 0;
    registry.emplace<SpriteRender>(entity, sprite);

    Hitbox hitbox;
    hitbox.shape = HitboxShape::Circle;
    hitbox.radius = spawn.params.hitboxRadius;
    registry.emplace<Hitbox>(entity, hitbox);

    // Collides with walls (hits resolved by the swept raycast, not push-out — the
    // Projectile tag excludes it from collision_step's solid resolution).
    registry.emplace<CollisionLayer>(entity, CollisionLayer{layer::Projectile, layer::Wall});
    registry.emplace<ForceResponse>(entity,
                                    ForceResponse{spawn.params.deflectable
                                                      ? ForceResponseType::Deflectable
                                                      : ForceResponseType::Rigid});
    return entity;
}

} // namespace sc
