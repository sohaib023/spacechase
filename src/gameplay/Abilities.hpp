#pragma once

// Code-defined abilities (brief §6.3): abilities are NOT player content, so they
// live as C++ structs built by a code registry rather than JSON (ships still load
// from JSON). The composition pipeline (def -> modifiers -> spawns) is unchanged;
// only the data source is code. The AbilityLoadout component carries a ship's
// resolved slots (texture handles included so FireSystem stays GPU-free).

#include "resources/TextureHandle.hpp"

#include <glm/glm.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace sc {

enum class AbilityCategory { Projectile, Beam };

struct ProjectileParams {
    float speed = 18.0f;          // m/s along facing
    float damage = 25.0f;         // per hit (every bounce hit deals it again)
    float maxRange = 40.0f;       // meters of travel before despawn (<= 0 = no limit)
    float lifetime = 0.0f;        // optional seconds backstop (0 = none)
    bool bounce = false;          // ricochet off Reflective surfaces?
    int maxBounces = -1;          // when bouncy: bounce limit (-1 = unlimited)
    float hitboxRadius = 0.15f;   // meters (collision/raycast)
    glm::vec2 spriteSize{0.4f, 0.4f};
    std::string sprite;           // albedo path (relative to assets dir)
    std::string normal;           // optional normal map
    bool deflectable = true;      // force-field response (deflector can repel it)
};

struct BeamParams {
    float damage = 40.0f;         // damage PER SECOND (continuous, scaled by dt)
    float maxDist = 30.0f;        // range: total path length in meters, across bounces
    bool bounce = false;          // reflect off Reflective surfaces?
    int maxBounces = 0;           // when bouncy: bounce limit (-1 = unlimited)
    float width = 0.12f;          // meters (viz)
    glm::vec3 color{1.0f, 0.4f, 0.2f};
};

struct AbilityDef {
    std::string id;
    std::string displayName;
    AbilityCategory category = AbilityCategory::Projectile;
    float cooldown = 0.2f;        // seconds between shots
    ProjectileParams projectile;
    BeamParams beam;
};

enum class ModifierKind { Multifire, Quickfire, Autofire };

struct ModifierDef {
    ModifierKind kind = ModifierKind::Multifire;
    float a = 0.0f; // kind-specific (Multifire: extra shots; Quickfire: cooldown mult)
    float b = 0.0f; // kind-specific (Multifire: total spread rad; Quickfire: speed mult)
};

// Per-shot parameters folded from an AbilityDef + its modifiers. fire_step starts
// from the ability's defaults, folds each modifier via apply_modifier, then emits
// shots accordingly. Adding a modifier behavior = a new ModifierKind + a case here,
// never a new firing path.
struct FireContext {
    int count = 1;            // shots per trigger
    float spread = 0.0f;      // total angular spread (radians), centered on facing
    float speedMult = 1.0f;   // projectile speed multiplier
    float cooldownMult = 1.0f; // cooldown multiplier (<1 = faster)
    bool autofire = false;    // hold-to-fire instead of one-per-press
};

void apply_modifier(FireContext& context, const ModifierDef& modifier);

// One equipped ability bound to a fire action, with its resolved sprite handles
// and live cooldown state.
struct AbilitySlot {
    AbilityDef ability;
    std::vector<ModifierDef> modifiers;
    std::string action = "fire1";  // "fire1" / "fire2"
    float cooldownRemaining = 0.0f;
    bool wasHeld = false;          // for rising-edge detection (non-autofire)
    TextureHandle projAlbedo = kInvalidTexture;
    TextureHandle projNormal = kInvalidTexture;
};

// Component: a ship's equipped abilities.
struct AbilityLoadout {
    std::vector<AbilitySlot> slots;
};

// Code registry: look an ability up by id (nullptr if unknown).
const AbilityDef* find_ability(std::string_view id);

} // namespace sc
