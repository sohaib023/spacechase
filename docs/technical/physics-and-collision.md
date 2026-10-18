# Physics & Collision

Movement, the custom top-down collision layer, raycasting, and pseudo-3D falling.
Code: `src/physics/` (pure geometry), `src/systems/{Movement,Collision,FloorAltitude}System.*`,
`src/gameplay/{Level,FallConfig}.hpp`. Force fields are in
[`abilities-and-forces.md`](abilities-and-forces.md).

No general physics engine. Everything is simple, deterministic-leaning code we own.

---

## 1. Units & conventions

- **Meters, seconds, radians.** World +Y is up. Rotation 0 faces +Y; forward =
  `(-sin θ, cos θ)`. The renderer converts with `Camera2D.pixelsPerMeter = 48`.
- **Continuous space, not a tile grid** (a user decision). Walls, floor regions and pits are
  arbitrary rectangles in meters.
- Fixed step `dt = 1/60 s`. All physics runs inside the fixed-step loop.
- `src/physics/` depends only on glm + `ecs/` (no SDL), so it's fully unit-testable.

## 2. Ship movement (`movement_step`)

Over `Transform + Velocity + ShipStats + ShipControl`, per step:
```
rotation += turn · turnSpeed · dt
velocity += forward(rotation) · thrust · acceleration · dt
velocity -= velocity · drag · dt                 // linear drag = "slipperiness"
if |velocity| > maxSpeed: velocity *= maxSpeed / |velocity|
position += velocity · dt
```
Player ship defaults (`player_ship.json`): acceleration 40 m/s², maxSpeed 20 m/s,
turnSpeed 3.2 rad/s, drag 1.2. Projectiles use their own `projectile_move_step`
(straight line, no drag).

## 3. Shapes & narrow phase (`physics/Shapes.hpp`, `physics/Overlap.*`)

- `Circle{center, radius}`, `AABB{center, halfExtents}` (+ `min()`/`max()`),
  `Convex{verts[≤8], count}` (`kMaxHitboxVerts = 8`), `rotate_point(p, cos, sin)`.
- `Hitbox` component: shape `Circle | AABB | Polygon` (convex local vertices, rotated by the
  entity's rotation), plus `offset`. Fixed-capacity array keeps it trivially copyable.
  - Player ship: **polygon** triangle from its JSON (`hitbox.polygon`). Walls/crates: AABB.
    Projectiles: small circle.
- Overlap tests return `Manifold{hit, normal, penetration}`; **the normal pushes A out of B**:
  `circle_vs_circle`, `circle_vs_aabb`, `aabb_vs_aabb`, **`convex_vs_convex` (SAT)**,
  **`circle_vs_convex`**.
- `CollisionSystem::collide` dispatches any pair. AABBs and polygons are converted to world-space
  `Convex` (rotated, so a rotated AABB becomes an oriented box) and go through SAT. Circle pairs use
  the circle paths.

## 4. Broadphase (`physics/SpatialHash`)

- Uniform grid over meter space, `cellSize = 4 m` (`kCellSize` in `CollisionSystem.cpp`).
- `insert(entity, aabb)`, `query(aabb, out)`, `clear()`. Query results are **sorted by entity id
  and de-duplicated**, giving a stable order and therefore deterministic resolution.
- **Rebuilt every fixed step** from all colliders (`Transform + Hitbox + CollisionLayer`). Fine at
  current counts. Persisting/incremental updates are a later optimization.

## 5. Collision filtering (`CollisionLayer`)

`layer` (what I am) + `mask` (what I resolve against), bitmasks in namespace `layer`:
`Ship = 1<<0`, `Wall = 1<<1`, `Projectile = 1<<2`. Ships: `{Ship, Wall}`. Walls: `{Wall, 0}` (static, never initiate resolution).
Projectiles: `{Projectile, Wall}`. Ships don't currently collide with each other.

## 6. Collision response (`collision_step`)

"Move, then push out" (discrete), run **after** `movement_step`:

1. Rebuild the hash.
2. Iterate dynamic movers (`Transform + Velocity + Hitbox + CollisionLayer`) in **id order**,
   skipping `StaticTag` and **`Projectile`** (bullets get no push-out; they're resolved by the swept
   raycast, see [`abilities-and-forces.md`](abilities-and-forces.md)).
3. Up to `kMaxIterations = 3` times: query the hash with the mover's bounds **expanded by this
   step's displacement** (swept broadphase, against tunneling), test each mask-matching candidate,
   take the **deepest** contact, then:
   - push out: `position += normal · (penetration − slop)` (`kSlop = 0.001 m` skin to avoid jitter),
   - slide: if `v·n < 0`, `v -= (v·n) n` (cancel the inward component: stop head-on, slide when
     glancing, **no bounce**).

Because `snapshot_prev_transforms` ran before movement, rendering interpolates toward the
*corrected* position, so the ship is never drawn inside a wall.

Limits: discrete resolution plus swept broadphase. Very fast movers vs thin walls could still
tunnel. Keep walls ≥ 1 m thick. Full CCD/sub-stepping is out of scope.

## 7. Raycast (`physics/Raycast`)

- Pure tests: `ray_vs_aabb` (slab method, returns entry `t` + face normal), `ray_vs_circle`.
- `raycast(registry, origin, dir, maxDist, mask) → RayHit{hit, t, point, normal, entity}`: nearest
  hit among colliders whose `layer` is in `mask`. `dir` is normalized internally.
- **Limitations:** it iterates every collider (O(N), no hash acceleration yet). AABB hitboxes are
  tested **axis-aligned** (rotation ignored). Polygon hitboxes are treated as a **circle of
  `Hitbox.radius`**. Fine today since only walls/crates (unrotated AABBs) are raycast targets.
- Consumers: the debug ray (from the ship's nose), projectile hits (swept segment), the beam.

## 8. Floor, void & pseudo-3D falling

- `Level{floorRects}` (`gameplay/Level.hpp`). `has_floor(p)` = point inside any floor rect. **Void
  = everything else.** Single floor layer for now.
- `Altitude{z, vz, falling, visualScale}` on ships. `FallConfig` (App-owned, panel-tunable):
  `gravity 18 m/s²`, `hoverHeight 1 m`, `deathZ 0`, `recoverRate 6 m/s`, `controlWhileFalling`.
- `floor_altitude_step` per ship:
  - **over floor:** `vz = 0`, rise toward `hoverHeight` at `recoverRate` (recovery);
    `falling = z < hoverHeight`.
  - **over void:** `vz -= gravity·dt; z += vz·dt; falling = true`. If `z ≤ deathZ` → death:
    currently **respawn at the level spawn** with zero velocity (placeholder for real death).
  - `visualScale = 0.4 + 0.6 · clamp((z − deathZ)/(hoverHeight − deathZ))`, the shrinking
    fall-away cue.
- `apply_fall_control` (before movement): if `controlWhileFalling` is false, zero the thrust of
  falling ships (turning still works). Flag kept on purpose for design testing.

## 9. Determinism notes

Toward future netcode: fixed step, fixed system order, id-sorted iteration where order affects
results (hash queries, mover order, projectile hits), no randomness yet, single-precision floats.
Not bit-exact across machines. If lockstep is ever chosen, revisit (see open questions in
[`../game-design/game-design.md`](../game-design/game-design.md)).

## 10. Planned

Multi-floor layers (fall to a lower floor instead of dying, M6) · portals (M6) · hash-accelerated
raycast · rotated-AABB/polygon raycast · ship-vs-ship collision when there are other ships ·
persistent spatial hash if entity counts demand it.
