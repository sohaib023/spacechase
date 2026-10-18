# M5 — Abilities, Projectiles & Force Fields

**Status:** ✅ done · **Goal:** the firing pipeline (brief §6.3) and the unified force-field system
(brief §6.4) as a **vertical slice** that proves the pipeline generalizes, not every ability.
It also closes the M3 loop: weapons damage and destroy crates.

## Decisions locked

| Decision | Choice |
|---|---|
| Scope | Projectile ability (bullets; missiles = parameter variants), machine-gun **beam** via raycast, modifiers **multifire / quickfire / autofire**, the **force-field** system (level wind zones), and a **Deflector** defensive ability. Deferred: deployables/mines, chi/warp, shield, homing (no targets yet). |
| Deflector | A **ship ability** (toggle, in the debug panel) that reuses the force-field system, **not** a level force field, which would duplicate it. |
| Force response | **Every collidable carries an explicit `ForceResponse = Rigid | Deflectable`**, an ECS component set by every factory (ships/projectiles Deflectable, walls/crates Rigid). Fields only push Deflectable. Chosen over implicit tag presence. |
| Hit response | **Per-projectile swept raycast** (`PrevTransform → Transform` vs walls each step): tunneling-proof, reuses `raycast()`. `collision_step` stays untouched (projectiles excluded from push-out). |
| Ability definitions | **Code-defined** C++ structs + registry (`gameplay/Abilities`), not JSON. Abilities aren't player content. Ships stay JSON; force fields are data in the level. |
| Structural changes | All create/destroy **deferred to one `commit_deferred` step**, the determinism + view-safety keystone. |

## Design summary

- **Components:** `Projectile`, `Lifetime`, `Damage`, `Owner{entity, team}`, `ForceField`,
  `ForceResponse`, `Deflector`, `BeamRequest`; `CollisionLayer::Projectile`.
- **Abilities:** `AbilityDef` (category, cooldown, projectile/beam params), `ModifierDef`, code table +
  `find_ability`; `AbilityLoadout` of slots with pre-resolved sprite handles.
- **Input:** `fire1` (Space), `fire2` (Shift).
- **Systems & pipeline:**
  ```
  snapshot_prev → input_apply → fire_step → apply_fall_control → movement_step
  → projectile_move_step → forcefield_step → collision_step → projectile_hit_step
  → beam_step → floor_altitude_step → lifetime_step → commit_deferred
  ```
- **Modifiers** fold into a `FireContext` via `apply_modifier`. New modifier = new kind + one case.

Full current description: [`../technical/abilities-and-forces.md`](../technical/abilities-and-forces.md).

## Sub-steps (each built, tested, committed)

| Step | Commit | Content |
|---|---|---|
| M5.1–5.2 | `3b4e942` | Components + `ForceResponse` on all factories; force fields (radial/directional, falloff), level `forceFields`, wind zone in the test level, deflector toggle. `test_forcefield`. |
| M5.3 | `b313efb` | Code abilities + loadout, `fire_step`, `Deferred` + `commit_deferred`, `ProjectileFactory`, `projectile_move_step`, `lifetime_step`, bullet sprite, Projectile guard in collision. `test_fire`, `test_lifetime`. |
| M5.4 | `7d32bc0` | `projectile_hit_step` (swept raycast, damage, owner filter, crate destruction). `test_projectile_hit`. |
| M5.5 | `acd1bbb` | `FireContext` + `apply_modifier`; semi-auto default vs autofire; panel toggles. `test_abilities`. |
| M5.6 | `da47ebd` | `BeamRequest` + `beam_step` + beam viz (fire2). `test_beam`. 61/61 tests. |

## Risks called out

Mid-iteration create/destroy (→ deferred queues) · spawn streak (`PrevTransform == Transform`) ·
swept-raycast cost O(projectiles × colliders) · owner self-hit (filter + muzzle offset) · beam damage
is per second · projectiles must not be shoved by collision · cooldowns tick in the fixed step.

## Outcome

Delivered. Deviations from the plan text:
- The deflector is handled **directly** by `forcefield_step` (it reads active `Deflector`s as
  radial sources) instead of adding/removing a `ForceField` component on the ship. Same behavior,
  no structural change.
- **Default fire is semi-auto** (rising edge). Autofire is the modifier.
- `BeamParams.color` is stored but not rendered (the renderer has no per-draw tint yet).
- `fire_step` reads `InputActions` for `PlayerControlled` ships directly. An AI/network fire-intent
  component is future work.
- Not yet verified by playtest at the time of writing, only by unit tests.
