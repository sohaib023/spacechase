#include "gameplay/Abilities.hpp"

#include <vector>

namespace sc {

namespace {

// The code-defined ability table. Adding an ability = a new entry here; adding a
// modifier behavior = a new ModifierKind + apply_modifier lines (FireSystem) — the
// firing pipeline itself never grows a new path.
std::vector<AbilityDef> build_abilities() {
    std::vector<AbilityDef> table;

    AbilityDef blaster;
    blaster.id = "blaster";
    blaster.displayName = "Blaster";
    blaster.category = AbilityCategory::Projectile;
    blaster.cooldown = 0.18f;
    blaster.projectile.speed = 24.0f;
    blaster.projectile.damage = 25.0f;
    blaster.projectile.lifetime = 2.0f;
    blaster.projectile.hitboxRadius = 0.15f;
    blaster.projectile.spriteSize = {0.5f, 0.5f};
    blaster.projectile.sprite = "sprites/bullet.png";
    table.push_back(blaster);

    AbilityDef machinegun;
    machinegun.id = "machinegun";
    machinegun.displayName = "Machine Gun";
    machinegun.category = AbilityCategory::Beam;
    machinegun.cooldown = 0.0f;
    machinegun.beam.damage = 40.0f;
    machinegun.beam.maxDist = 30.0f;
    machinegun.beam.width = 0.12f;
    machinegun.beam.color = {1.0f, 0.4f, 0.2f};
    table.push_back(machinegun);

    return table;
}

} // namespace

void apply_modifier(FireContext& context, const ModifierDef& modifier) {
    switch (modifier.kind) {
    case ModifierKind::Multifire:
        context.count += static_cast<int>(modifier.a); // extra shots
        context.spread = modifier.b;                   // total spread radians
        break;
    case ModifierKind::Quickfire:
        context.cooldownMult *= modifier.a;            // <1 fires faster
        if (modifier.b > 0.0f) {
            context.speedMult *= modifier.b;
        }
        break;
    case ModifierKind::Autofire:
        context.autofire = true;
        break;
    }
}

const AbilityDef* find_ability(std::string_view id) {
    static const std::vector<AbilityDef> table = build_abilities();
    for (const AbilityDef& def : table) {
        if (def.id == id) {
            return &def;
        }
    }
    return nullptr;
}

} // namespace sc
