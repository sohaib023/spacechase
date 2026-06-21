# Space Chase — Codebase Guide (Milestone 3 delta)

> Builds on `codebase-guide-m1.md` and `codebase-guide-m2.md`. Covers what **M3**
> added: continuous meter units, a collision/physics layer, walls, raycast,
> pseudo-3D falling, multi-texture rendering, and data-driven levels.

---

## 1. What M3 changed

- **Units are now meters** (was 1 unit = 1 pixel). `Camera2D` has a `pixelsPerMeter`;
  ship stats are physical (m/s², m/s); all sizes are meters.
- **Rendering does multiple textures.** A `TextureRegistry` owns textures and hands out
  `TextureHandle`s; the sprite batch flushes one draw per texture. `SpriteRender` holds a handle.
- **A physics layer** (`src/physics/`): shapes, overlap manifolds, a spatial-hash broadphase,
  and raycasting — all pure logic, unit-tested.
- **Collision**: the ship (circle) stops/slides against walls (AABB), resolved each fixed step.
- **Walls** (static + destructible with `Health`) and the **floor/voids** load from a **JSON level**.
- **Pseudo-3D falling**: ships have an `Altitude`; over void, gravity pulls them down, with a
  configurable chance to steer back to safety.

---

## 2. New layers / files

### `src/physics/` (pure logic — glm + ecs only, no SDL)
- `Shapes.hpp` — `Circle`, `AABB` (center + half-extents), all in meters.
- `Overlap.{hpp,cpp}` — `circle_vs_aabb` / `circle_vs_circle` / `aabb_vs_aabb` → `Manifold` (normal + penetration).
- `SpatialHash.{hpp,cpp}` — uniform-grid broadphase; `insert`/`query`/`clear`; query results id-sorted (deterministic).
- `Raycast.{hpp,cpp}` — `ray_vs_aabb` / `ray_vs_circle` + a registry-level `raycast()` (nearest masked hit).
- `ForceApi.hpp` — stub seam for M5 force fields.

### `src/resources/` (rendering resources)
- `TextureHandle.hpp` — the handle type + `kInvalidTexture`.
- `TextureRegistry.{hpp,cpp}` — owns textures, dedups by path, `load`/`get`/`destroy_all`.

### Components (`src/components/`)
`Hitbox` (Circle/AABB), `CollisionLayer` (bitmask filter), `StaticTag`, `Wall` (destructible flag),
`Health`, `Altitude` (z, vz, falling, visualScale).

### Systems (`src/systems/`)
- `CollisionSystem` (`collision_step`) — rebuild hash → swept broadphase → circle-vs-AABB →
  resolve (push-out + velocity slide), after `movement_step`.
- `FloorAltitudeSystem` (`apply_fall_control`, `floor_altitude_step`) — the falling model.

### Data + gameplay
- `data/WallDef.hpp` + `DefinitionRegistry` walls (`load_walls_from_dir`/`find_wall`; the loader is
  now a shared `load_defs` template).
- `data/LevelDef.hpp` + `data/LevelLoader.{hpp,cpp}` — parse a level JSON → `LevelData`
  (floor/void rects, spawn, wall instances). GPU-free.
- `gameplay/Level.hpp` — runtime floor (`floorRects` minus `voidRects`) + `has_floor(point)`.
- `gameplay/FallConfig.hpp` — gravity / hover height / deathZ / `controlWhileFalling` flag.
- `gameplay/WallFactory.{hpp,cpp}` — composes a wall entity (+ `Health` if destructible).

### Assets
`sprites/wall.png`, `crate.png`, `floor.png`, `void.png`, `debug_white.png`;
`definitions/walls/{wall,crate}.json`; `levels/test_m3.json`.

---

## 3. Updated fixed-step pipeline (`App::run`)

```
while accumulator >= 1/60:
    snapshot_prev_transforms(reg)
    input_apply(reg, actions)                      // now inside the loop
    apply_fall_control(reg, fallConfig)            // lock thrust if falling & flag off
    movement_step(reg, 1/60)                       // integrate (tentative)
    collision_step(reg, 1/60)                      // push out of walls + slide
    floor_altitude_step(reg, level, fallConfig, spawn, 1/60)  // gravity / recover / death
    accumulator -= 1/60
```
Render order each frame: **floor rects → void rects → entities (render_collect) → debug ray.**
Collision resolves *after* integration (discrete "move then push out"); `snapshot_prev_transforms`
stays before movement so the render interpolates toward the corrected position (never into a wall).

---

## 4. Key mechanics

**Collision response (circle vs AABB):** closest-point test → `Manifold{normal, penetration}`;
resolve = push out along the normal + cancel the inward velocity component (stop head-on, slide
glancing; no bounce). Iterates a few times for corners with a small skin-width slop. Swept
broadphase (query expanded by step displacement) guards against fast-mover tunneling.

**Pseudo-3D falling:** a ship hovers at `hoverHeight`. Over a void it loses altitude under
`gravity`; if it returns over floor before altitude hits `deathZ` it recovers (rises back); reaching
`deathZ` over void is fatal (respawn at spawn, for M3). `controlWhileFalling` (panel toggle) gates
whether thrust works mid-fall. A `visualScale` shrinks the sprite as it falls (rendering cue).

**Data-driven level:** `LevelLoader` parses floor/void rects + wall instances (referencing
`WallDef`s by id) + the spawn; `App` instantiates walls (resolving textures) and spawns the player.
This is the M3 subset of the eventual editor level format (brief §7).

---

## 5. M3 simplifications & what M4 changes

| M3 (now) | M4 (next, per brief §9) |
|---|---|
| Flat sprites, no lighting | **normal-mapped 2.5D lighting** (point/dir lights with height; normals rotated with sprite) — introduces the SDL_shadercross build step |
| Spatial hash rebuilt every step | persist/optimize if entity counts grow (M5 projectiles) |
| Fall death = respawn at origin | real death/scoring later |
| Single floor layer | multi-floor + parallax (M6) |
| Hand-compiled DXIL shaders | shadercross emits SPIR-V/DXIL/MSL from one HLSL source |
