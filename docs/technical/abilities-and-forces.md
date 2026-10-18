# Abilities, Projectiles & Force Fields

The firing pipeline (projectiles, beam, modifiers), the projectile lifecycle, the unified
force-field system, and the deferred spawn/despawn mechanism.
Code: `src/gameplay/{Abilities,Spawning,ProjectileFactory}.*`,
`src/systems/{Fire,ProjectileMove,ProjectileHit,Beam,Lifetime,Range,ForceField}System.*`,
`src/physics/ForceApi.hpp`.

---

## 1. Abilities are code-defined

`gameplay/Abilities.{hpp,cpp}`. Unlike ships/walls/levels, abilities are **C++ structs in a code
table**, not JSON. They're a balance surface developers own, not player content. The *composition*
design still applies: an ability is data + modifiers folded into spawns, never a subclass.

```cpp
AbilityDef { id, displayName, category (Projectile | Beam), cooldown,
             ProjectileParams { speed, damage, maxRange, lifetime /*0 = none*/,
                                bounce, maxBounces /*-1 = unlimited*/, hitboxRadius,
                                spriteSize, sprite, normal, deflectable },
             BeamParams       { damage /*per second*/, maxDist /*range*/, bounce,
                                maxBounces, width, color } }
ModifierDef { kind (Multifire | Quickfire | Autofire), a, b }
const AbilityDef* find_ability(id);   // "blaster" (projectile), "machinegun" (beam)
```

Current table: **blaster** = bouncy, unlimited bounces, 40 m range, no lifetime.
**machinegun** = beam, 30 m range, bouncy with 2 bounces.

**Loadout:** `AbilityLoadout { vector<AbilitySlot> }` component on a ship. An `AbilitySlot` holds
a copy of the `AbilityDef`, its `modifiers`, the bound `action` (`"fire1"` / `"fire2"`), live
`cooldownRemaining` / `wasHeld`, and **pre-resolved texture handles** for its projectile sprite,
so `fire_step` never touches the GPU. `App::init` builds the player's loadout: fire1 = blaster,
fire2 = machinegun.

## 2. Modifiers: the fold

```cpp
FireContext { count = 1, spread = 0, speedMult = 1, cooldownMult = 1, autofire = false }
apply_modifier(ctx, mod):
  Multifire  → count += a;  spread = b (total radians, centered on facing)
  Quickfire  → cooldownMult *= a;  if b > 0: speedMult *= b
  Autofire   → autofire = true
```
Each shot, `fire_step` starts from a default `FireContext`, folds every modifier in the slot, then
fires from the result. **Adding a modifier = a new `ModifierKind` + one `apply_modifier` case**, never
a new firing path. Modifiers stack (applied in list order). The Ship debug panel's multifire /
quickfire / autofire checkboxes rebuild the fire1 slot's modifier list live (values 2 extra shots /
0.35 rad; cooldown ×0.4; autofire).

## 3. `fire_step` (input → intent)

For each `PlayerControlled` ship with `AbilityLoadout + Transform`, for each slot:
1. `cooldownRemaining = max(0, cooldownRemaining − dt)`. Cooldowns tick in the **fixed step**, so
   cadence is frame-rate independent.
2. `held` = the slot's action from `InputActions` (fire1 = Space / left mouse, fire2 = Shift /
   right mouse; mouse bindings sidestep keyboard ghosting).
3. **Beam slot:** copy `held` + beam params into the ship's `BeamRequest`; done.
4. **Projectile slot:** fold modifiers. Trigger = `held` if autofire, else **rising edge**
   (`held && !wasHeld`, semi-auto default). If triggered and off cooldown, queue `count`
   `ProjectileSpawn`s spread evenly across `spread`, each with
   `position = ship + dir · 1 m` (muzzle offset clears the hull), `velocity = dir · speed · speedMult`,
   `rotation`, `owner = ship`. Then set `cooldownRemaining = cooldown · cooldownMult`.
5. `wasHeld = held`.

Firing is currently tied to `PlayerControlled` + `InputActions`. AI/network shooters later should
write a per-ship fire intent component instead (same pattern as `ShipControl`).

## 4. Deferred spawn/despawn (`gameplay/Spawning`)

- `Deferred { vector<ProjectileSpawn> spawns; vector<Entity> despawns; }`, owned by `App`.
- Systems only **queue**. **`commit_deferred(registry, deferred)` is the single structural-change
  point**, called last in the fixed step: spawns via `spawn_projectile`, then destroys despawns
  (each guarded by `registry.valid`, so a duplicate, e.g. a bullet that hit *and* expired the same
  step, is harmless), then clears both queues.
- Why: creating/destroying while iterating EnTT views is unsafe, and a single commit point keeps the
  order deterministic (brief §8).

## 5. Projectile lifecycle

`ProjectileFactory::spawn_projectile` emplaces: `Transform` + **equal `PrevTransform`** (no
first-frame streak), `Velocity`, `Projectile` tag, `Damage`, `Owner{entity, team}`,
`SpriteRender`, `Hitbox` (circle), `CollisionLayer{Projectile, Wall}`, `ForceResponse`
(Deflectable unless `params.deflectable == false`), plus three **independent, optional
traits**: `Range{remaining}` (if `maxRange > 0`), `Lifetime` (if `lifetime > 0`) and
`Bounce{remaining}` (if `bounce`; −1 = unlimited). No `Bounce` component = never bounces.

Per step:
1. `projectile_move_step`: `position += velocity · dt` (straight line; separate from ship movement
   because bullets have no stats/drag/turn).
2. `forcefield_step` may bend them (wind, deflectors).
3. `collision_step` **skips** `Projectile` entities (one guard line), so there's no push-out.
4. **`projectile_hit_step` (swept raycast + ricochet):** for each bullet in id order, walk the
   step's path `PrevTransform → Transform` segment by segment against `layer::Wall`. Each hit
   that isn't the owner subtracts `Damage` from the target's `Health` and queues the target at
   ≤ 0 (**this destroys crates**). Then the bullet **ricochets** if it has `Bounce` with bounces
   left *and* the surface is `Reflective`: velocity mirrored `v − 2(v·n)n` (same speed), the rest
   of the path is swept again (≤ 8 hits per step as a corner guard), and `Transform`/`Velocity`/
   rotation end where the path ends. Otherwise it's **absorbed** (queued for despawn). A zero hit
   normal (ray started inside a box) counts as absorbed. Sweeping makes fast bullets
   tunneling-proof.
5. `lifetime_step`: `Lifetime.remaining −= dt`; at ≤ 0 queue despawn.
6. **`range_step`**: `Range.remaining −= |velocity| · dt`; at ≤ 0 queue despawn. **Independent of
   bouncing** (the bounce code never touches `Range`). Since bounces never change speed, the path
   length per step is exactly `speed · dt` whether or not the bullet ricocheted. A bullet can
   overshoot its range by at most one step's travel (~0.4 m at 24 m/s).
7. `commit_deferred` applies it all.

A ricocheting bullet's render interpolation (`PrevTransform → Transform`) cuts the corner for one
sub-step. That's invisible at 60 Hz.

Rendering is automatic: projectiles have `SpriteRender` + transforms, so `render_collect` draws
them (`sprites/bullet.png`).

## 6. Beam: machine gun (`systems/BeamSystem`)

- `BeamRequest{active, damage, maxDist, width, color}` component on every ship (inactive by
  default; primitive fields so it doesn't depend on `gameplay/`). `fire_step` sets it.
- `beam_step(registry, viz, deferred, dt)`: clears `viz`; for each active request, traces a
  **polyline** from `ship + facing · 1 m` along facing vs `layer::Wall`. `maxDist` is the beam's
  **range, shared by all segments**. Each segment ends at a hit: `Health −= damage · dt`
  (**damage is per second**, applied to every entity the beam touches, again for each touch), and
  the target is queued for despawn at ≤ 0. The beam continues (reflected) only off a
  `Reflective` surface while it's bouncy with bounces left (`bounce`, `maxBounces`, −1 =
  unlimited). It stops at absorbent surfaces, the bounce limit, or when range runs out. There's a
  hard cap of 16 segments. Every segment is appended as a `BeamSegment{start, end, width, color}`.
- `App` owns the `BeamVizList` and draws each segment as a thin **unlit** quad after the world.
  Viz reflects the last fixed sub-step of the frame.
- Single ray today. The planned **cone** machine gun (partly blocked by walls) = N thin rays.

## 6b. Surfaces: the hit interaction rule

What happens on a hit = f(**projectile/beam traits**, **surface response**):

| | Surface `Absorbent` | Surface `Reflective` |
|---|---|---|
| Not bouncy (no `Bounce` / `bounce = false`) | damage, stop | damage, stop |
| Bouncy, bounces left | damage, stop | **damage, reflect, continue** |
| Bouncy, limit reached | damage, stop | damage, stop |

- `SurfaceResponse{Absorbent | Reflective}` is set **explicitly on every wall** by `WallFactory`
  from the wall definition's `"surface"` field (default absorbent). Which walls are which is pure
  data. Currently `wall` is reflective and `crate` absorbent.
- No speed or damage loss on bounce.
- **Future: penetrating abilities (chi).** Chi is a semi-transparent space-warp projectile that
  passes *through* walls, losing speed and damage as it does. It will add a third projectile trait
  (e.g. `Penetrate{speedLoss, damageLoss}`) that overrides this table, handled in the same hit
  loop. Not built yet. See the planned abilities in
  [`../game-design/game-design.md`](../game-design/game-design.md).

## 7. Force fields: one system, many features (`systems/ForceFieldSystem`)

**Response is explicit on every collidable:** `ForceResponse{Rigid | Deflectable}`. Ships and
projectiles are Deflectable; walls and crates are Rigid. Fields only ever push **Deflectable**
entities. Rigid ones never move.

**Sources:**
- `ForceField{shape, dir, strength, range, falloff}` + `Transform`, placed from the level's
  `forceFields` array (wind zones).
- **Active `Deflector{active, strength, range}` on a ship.** The deflector is a *ship ability*, not
  a level object. `forcefield_step` treats an active deflector as a **radial, linear-falloff
  repulsion source** at the ship. Same code path, no duplicate system. Toggled in the Ship panel
  (strength/range sliders when active). It also pushes the ship's **own** outgoing bullets
  (intended), which speeds them up since they fly straight out. To see it *bend* bullets, use
  the panel's **"fire test volley at ship"** button (incoming, ownerless bullets).

**`forcefield_step(registry, dt)`**: for every source × every Deflectable target
(`Transform + Velocity + ForceResponse`), **skipping self**:
- **Radial:** within `range`, push along `(target − source)/dist` with magnitude
  `strength · falloff(dist)`. Positive strength repels.
- **Directional (wind):** within `range` of the source, push along `dir` with
  `strength · falloff(dist)`.
- Falloff: `None` = 1, `Linear` = `max(0, 1 − dist/range)`, `InverseSquare` = `1/(1 + dist²)`.
- Applied as `apply_impulse(velocity, force · dt)` (`physics/ForceApi.hpp`).

It runs **after movement and before collision**, so a field shoving a ship into a wall is resolved
the same step. Because it's a force (not a constraint) it can be **overcome**: enough thrust beats
a deflector near its edge, by design.

Cost: O(sources × deflectable targets) per step. Fine now; bucket via the spatial hash if counts
grow.

## 8. Recipes

- **New projectile weapon:** add an entry to `build_abilities()` (category `Projectile`, params,
  sprite path, `maxRange`, `bounce`/`maxBounces`) and add it to a loadout slot in `App::init`.
  That's all.
- **Make a surface reflective/absorbent:** set `"surface"` in its wall definition JSON.
- **Missile variant:** same as above with different speed/lifetime/sprite. Homing needs a `Homing`
  component + a small steering system (deferred until there are targets).
- **New modifier:** extend `ModifierKind`, add the `apply_modifier` case (and any new `FireContext`
  field + its use in `fire_step`), plus a test in `tests/test_abilities.cpp`.
- **New field shape (e.g. spiral):** add to `FieldShape`, compute its direction in
  `forcefield_step`, parse it in `LevelLoader`, test in `tests/test_forcefield.cpp`.
- **New ability category** (deployable, shield, warp): new `AbilityCategory` value, a branch in
  `fire_step` that writes an intent component or queues a spawn, and a dedicated system. Keep all
  entity creation going through `Deferred`.

## 9. Tests

`test_fire` (cadence, cooldown, owner, muzzle velocity), `test_abilities` (modifier fold +
stacking, multifire spread, quickfire cooldown, semi-auto vs autofire), `test_lifetime`
(decrement, commit, duplicate despawn), `test_range` (expiry, direction independence, a
ricocheting bullet expires after the same distance as a straight one), `test_projectile_hit`
(sweep damage, lethal double despawn, miss, owner filter, ricochet keeps speed, absorbent stop,
non-bouncy absorbed, bounce limit, damage on every bounce), `test_beam` (per-second damage, clip
at face, full length, lethal, reflection + shared range, absorbent stop, bounce limit with damage
per hit), `test_forcefield` (radial/directional/falloff, rigid skip, deflector on/off/self skip).

**Debug panel:** the Ship panel's *bounce* section toggles bounce, max bounces (−1…10) and range
for the blaster and the beam live.

## 10. Deferred / planned

Deployables (mines, laid lasers), field/warp (chi, including wall penetration with
speed/damage loss), shields, homing/heatseeking, cone beam,
enemy/AI shooters (fire-intent component), teams/PvP (`Owner.team` reserved), per-draw beam tint,
spatial-hash-accelerated raycasts.
