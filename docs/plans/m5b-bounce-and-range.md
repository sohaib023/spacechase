# M5b — Bouncing Projectiles/Beams, Surface Response & Max Range

**Status:** ✅ done · **Goal:** base bullets ricochet off reflective surfaces (damage on every
hit, no speed/damage loss), absorbent surfaces stop them, beams can bounce too, and every
projectile/beam has a max range. Everything composable per ability.

## Context

Requested after M5 playtesting. Bounce, bounce limit (or unlimited), and range must be per-ability
choices in the composable ability architecture. Which surfaces reflect is data. A future **chi**
ability (passes through walls, losing speed and damage) was noted for the docs only.

## Decisions locked

| Decision | Choice |
|---|---|
| Interaction rule | Hit → damage. Then reflect only if the projectile/beam is bouncy, has bounces left, **and** the surface is `Reflective`; otherwise stop. |
| Surface | New `SurfaceResponse{Absorbent | Reflective}`, explicit on every wall, from the wall def's `"surface"` field (default absorbent). `wall` reflective, `crate` absorbent (arbitrary, data). |
| Bounce | Per ability: `bounce` + `maxBounces` (−1 = unlimited). Projectiles carry a `Bounce` component only when bouncy. No speed/damage loss. |
| Range | Per ability `maxRange` (projectiles) / `maxDist` (beam, shared across segments). **Range is a separate attribute and system from bouncing**: `range_step` decrements `Range` by `|velocity|·dt`. Exact even for ricochets because speed never changes. Overshoot ≤ one step (~0.4 m), accepted. |
| Lifetime | Kept as an optional backstop (`lifetime = 0` → none). |
| Timestep | Stays fixed 60 Hz with render interpolation; 120 Hz later via `kFixedDt` if input latency matters. |

## Sub-steps

| Step | Commit | Content |
|---|---|---|
| 1 | `bb18aa9` | `SurfaceResponse`, `Range`, `Bounce`; WallDef `surface`; ability params; factory composition; `range_step`. Tests: surface parse, range expiry. |
| 2 | `4bd07ba` | Ricochet in `projectile_hit_step` (segment walk, ≤ 8 hits/step, mirrored velocity, zero-normal = absorb). Tests incl. bounce + range integration. |
| 3 | `ea70773` | Bouncing beam polyline in `beam_step` (shared range, ≤ 16 segments). Tests. |
| 4 | (docs commit) | Ship-panel bounce/range toggles; docs (technical, data schema, game design + chi note), this plan. |

## Outcome

Delivered as planned. 73/73 tests. The game starts cleanly; in-game feel still to be confirmed by
playtest. Known cosmetic limitation: a ricocheting bullet's render interpolation cuts the corner
for one sub-step.
