# Space Chase — Codebase Guide (Milestone 5 delta)

> Builds on `codebase-guide-m1..m4`. Covers what **M5** added: the data-driven
> firing pipeline (projectiles + machine-gun beam + modifiers), the unified
> force-field system (level wind + a ship deflector ability), and the deferred
> spawn/despawn keystone. It also closes the M3 loop — weapons now destroy crates.

---

## 1. What M5 changed

- **Weapons.** A ship has an `AbilityLoadout`; holding a fire action spawns bullets
  (raycast-hit) or fires a continuous **beam** (hitscan). Modifiers reshape firing
  without new code paths.
- **Force fields.** One system pushes any **Deflectable** object: level **wind**
  zones (data) and a ship **Deflector** ability (toggle) are both just field sources.
- **Force response is explicit.** Every collidable carries `ForceResponse` =
  `Rigid` or `Deflectable`. Fields move Deflectable only; Rigid never moves.
- **Deferred structural change.** All create/destroy is queued and applied at one
  `commit_deferred` step — never mid-iteration (determinism + view safety).

---

## 2. New components (`src/components/`, POD)

- `Projectile` — tag; excludes the entity from `collision_step` push-out.
- `Lifetime{remaining}` — seconds until auto-despawn.
- `Damage{amount}` — per-hit (projectile) damage.
- `Owner{entity, team}` — shooter, for the self-hit filter.
- `ForceField{shape, dir, strength, range, falloff}` — a field **source**.
- `ForceResponse{type}` — `Rigid` | `Deflectable`, on every collidable.
- `Deflector{active, strength, range}` — the ship's defensive ability.
- `BeamRequest{active, damage, maxDist, width, color}` — per-step beam intent
  (primitive fields, no gameplay dependency).
- `CollisionLayer` gained `Projectile = 1u<<2`.

## 3. Code-defined abilities (`src/gameplay/Abilities.{hpp,cpp}`)

Abilities are **not** player content, so they live as C++ structs, not JSON
(ships still load from JSON; force-fields are data in the level).

- `AbilityDef{id, category, cooldown, ProjectileParams, BeamParams}`; `find_ability(id)`
  returns entries from a code table (`blaster` projectile, `machinegun` beam).
- `AbilityLoadout` (component): a ship's `AbilitySlot`s. Each slot binds an ability
  to an action (`"fire1"`/`"fire2"`), carries its modifiers + live cooldown, and
  **pre-resolved texture handles** so `fire_step` stays GPU-free.
- `FireContext` + `apply_modifier`: per-shot params folded from the ability's
  defaults through each `ModifierDef`. Multifire → count + spread; Quickfire →
  cooldown/speed mults; Autofire → hold-to-fire. **Adding a modifier = a new
  `ModifierKind` + an `apply_modifier` case**, never a new firing path.

## 4. Deferred spawn/despawn (`src/gameplay/Spawning.{hpp,cpp}`)

- `ProjectileSpawn` record + `Deferred{spawns, despawns}` queues (App owns one).
- `commit_deferred` is the **only** structural-change point: it drains spawns via
  `spawn_projectile`, then destroys despawns (`valid()`-guarded, so duplicate
  despawns from "hit + expired same step" are safe).
- `ProjectileFactory::spawn_projectile` mirrors `ShipFactory`; `PrevTransform`
  starts equal to `Transform` (no first-frame streak).

## 5. New systems (`src/systems/`)

- `fire_step(registry, actions, deferred, dt)` — ticks cooldowns; for each
  `PlayerControlled` loadout slot, folds modifiers and either queues
  `ProjectileSpawn`s (spread by `FireContext`) or sets the ship's `BeamRequest`.
- `projectile_move_step(registry, dt)` — straight-line `Transform += Velocity*dt`
  (separate from `movement_step`; bullets have no momentum/drag/turn).
- `projectile_hit_step(registry, deferred)` — **swept raycast** of each bullet's
  `PrevTransform → Transform` segment vs `layer::Wall` (tunneling-proof). On a hit
  (skipping the owner): apply `Damage` to `Health`, queue the target for despawn at
  `Health<=0` (**destroys crates**), always queue the bullet for despawn.
- `beam_step(registry, viz, deferred, dt)` — for each active `BeamRequest`: raycast
  from the muzzle vs `layer::Wall`, apply `damage * dt` (**per-second**) to the hit
  target (queue despawn at zero), and append a `BeamSegment` to the App's
  `BeamVizList` (drawn as a thin unlit quad). Single ray; cone-blocking deferred.
- `forcefield_step(registry, dt)` — pushes every `Deflectable` target (`Transform +
  Velocity + ForceResponse`) from each field source, skipping self. Sources are
  explicit `ForceField` entities **and** any ship whose `Deflector.active` is true
  (radial repulsion). Radial/Directional shapes; None/Linear/InverseSquare falloff.

## 6. Fixed-step pipeline order (`App::run`)

```
snapshot_prev_transforms
input_apply
fire_step                 // queue spawns / set BeamRequest
apply_fall_control
movement_step             // ships
projectile_move_step      // bullets (straight-line)
forcefield_step           // push Deflectable (incl. active deflector)
collision_step            // ship push-out (projectiles guarded out)
projectile_hit_step       // swept raycast → damage + despawn
beam_step                 // hitscan → damage + viz
floor_altitude_step
lifetime_step             // expire projectiles
commit_deferred           // the ONLY create/destroy point
```

`collision_step` is otherwise **untouched** — one `Projectile` guard skips bullets
from push-out so they pass through to the swept raycast.

## 7. Data + input

- Level JSON gains a `"forceFields"` array (mirrors `"lights"`): shape, position,
  dir, strength, range, falloff → `Transform + ForceField` entities. `test_m3.json`
  has a directional **wind corridor**. The deflector is a ship ability, so there is
  no level deflector (no duplication).
- `InputActions` gained `fire1` (Space) / `fire2` (Shift). The Ship debug panel
  toggles the deflector and the weapon modifiers (multifire/quickfire/autofire),
  rebuilding the fire1 slot's modifier list live.
- `sprites/bullet.png` — procedural projectile sprite.

## 8. Tests

`test_forcefield` (radial/directional/falloff, rigid-skip, deflector
active/inactive/self-skip), `test_fire` (cadence/cooldown/owner/velocity),
`test_lifetime` (decrement/commit/dup-despawn), `test_projectile_hit`
(sweep/lethal/miss/owner-filter), `test_abilities` (modifier fold + stacking,
multifire/quickfire/autofire through `fire_step`), `test_beam` (per-second damage,
clip-at-face, full-length-miss, lethal despawn). **61/61 pass.**

## 9. Deferred to later milestones

Deployable/mines, chi/warp, shield, homing/heatseeking (need enemy targets),
cone-blocking beams, spatial-hash-accelerated raycast, multi-floor (M6),
editor/level-format-v2 (M7), SDL_shadercross.
