# Architecture Overview

How the code is organized, how a frame runs, and the conventions every module follows. Start here,
then read the module docs:

| Doc | Covers |
|---|---|
| [`rendering.md`](rendering.md) | GPU device, sprite batch, textures, lighting/shaders, camera |
| [`physics-and-collision.md`](physics-and-collision.md) | units, shapes, SAT, spatial hash, raycast, collision response, falling |
| [`data-and-content.md`](data-and-content.md) | JSON definitions, level format, registries, adding content |
| [`abilities-and-forces.md`](abilities-and-forces.md) | firing pipeline, modifiers, projectiles, beam, force fields, deferred spawn/despawn |

---

## 1. Stack

C++20 · **SDL3** (window, input, GPU API on D3D12) · **Dear ImGui** (debug UI) · **EnTT** (ECS) ·
**glm** (math) · **stb_image** (PNG decode) · **nlohmann/json** (content) · **doctest** (tests) ·
CMake + FetchContent + Ninja, compiled with **clang-cl**. Logging is a thin wrapper over `SDL_Log`
(no spdlog). Networking (ENet) and audio arrive in later milestones.

Anti-bloat rule (from the brief): every dependency must beat "could a small amount of our own code
do this?".

## 2. Load-bearing principles

1. **Composition over inheritance (ECS).** Entities are bags of POD components; behavior lives in
   systems. New behavior = new component + small system, never a subclass.
2. **Data-driven content.** Ships, walls and levels are JSON read by generic code. (Exception by
   design: abilities are code-defined. See [`abilities-and-forces.md`](abilities-and-forces.md).)
3. **One source of truth.** The runtime and the future editor read the same registries and level
   format.
4. **Fixed-timestep simulation, interpolated rendering** (60 Hz). Prerequisite for stable physics
   and netcode.
5. **Unify similar mechanics.** Wind, deflector and chi are all force fields.
6. **Multiplayer-shaped, implemented late.** Sim is self-contained (no globals), entity state is POD
   and replicable, structural changes are deferred and deterministic.
7. **Soft engine/game split.** Reusable layers vs game layers, but no general engine product.

## 3. Repository layout

```
CMakeLists.txt / CMakePresets.json   build (single exe + test exe; see §8)
cmake/            warnings.cmake (strict flags), imgui.cmake (builds ImGui from FetchContent)
assets/
  shaders/        *.hlsl sources + compiled/*.dxil|*.spv (committed; tools/compile_shaders.cmake)
  sprites/        PNG albedo + *_normal.png normal maps
  definitions/    ships/*.json, walls/*.json      (content as data)
  levels/         test_m3.json                    (the current test level)
  audio/          (empty, M8)
src/
  game/           main.cpp: entry point only
  core/           App (composition root + main loop), Log.hpp, Input.hpp
  render/         Renderer (sprite batch), Camera2D, CameraFollow, LightsUBO
  resources/      Texture, TextureRegistry/TextureHandle, stb_image_impl.cpp
  ecs/            Registry.hpp (EnTT seam), World.hpp
  components/     POD component structs, one per header
  systems/        free-function systems over the registry
  physics/        pure geometry: Shapes, Overlap (SAT), SpatialHash, Raycast, ForceApi
  data/           definition structs + DefinitionRegistry + LevelLoader (JSON stays in .cpp)
  gameplay/       factories (Ship/Wall/Projectile), Abilities, Spawning, Level, FallConfig
  math/ net/ editor/ ui/   placeholders (.gitkeep) for later milestones
tests/            doctest unit tests (test_*.cpp)
tools/            hand-run scripts (shader compile, normal-map generation)
docs/             all project documentation (this folder)
```

**Headers live next to their sources** (`Foo.hpp` + `Foo.cpp` in the same folder). For an
application (not a library with a public include API) feature-colocation beats an `include/`/`src/`
split: you open one folder to see a module. Header-only types (most components, `Camera.hpp`,
`Log.hpp`) have no `.cpp`.

## 4. Layering & dependency direction

```
game/main ──► core/App   (composition root: owns everything, wires layers together)
                 │
   ┌──────────┬──┴────────┬───────────┬────────────┬──────────┐
 systems/   gameplay/    data/       render/     resources/   ecs/
   │           │           │           │
   └───────────┴─────► components/ ◄───┘          physics/ (glm + ecs only, no SDL)
                            │
              glm · EnTT ········· SDL3 · stb · ImGui (render/resources/core only)
```

- Dependencies flow **game → engine layers → core**, never upward.
- `core/App` is the **only** place sibling layers meet.
- `physics/`, `components/`, `systems/` and `data/` are **GPU-free**, which is what lets the test
  executable compile them without a device.
- Known, accepted bend: `systems/FireSystem` includes `gameplay/Abilities.hpp` (the loadout
  component carries ability data). Fine while we're a single target; revisit if layers become
  libraries.

## 5. The ECS model

- `ecs/Registry.hpp`: `sc::Registry = entt::registry`, `sc::Entity = entt::entity`. The **only** EnTT
  include seam.
- `ecs/World.hpp`: `World { Registry registry; Entity playerShip; }`, owned by value in `App`. No
  global registry; the sim is self-contained (replication-friendly).
- **Components** (`components/`): plain structs, no logic, no pointers to GPU objects (textures are
  `TextureHandle` integers). Design attributes are **explicit components/enums** rather than implied
  by tag presence (e.g. `ForceResponse{Rigid|Deflectable}` on every collidable).
- **Systems** (`systems/`): free functions `xxx_step(Registry&, …, float dt)`. No scheduler
  framework; the "pipeline" is the explicit ordered call list in `App::run` (§6).
- **Factories** (`gameplay/*Factory`): compose an entity from a definition. They always set
  `PrevTransform == Transform` so the first interpolated frame doesn't streak.
- **Structural changes are deferred.** Mid-step systems never create/destroy entities; they push
  onto `Deferred{spawns, despawns}` and `commit_deferred` applies them at the end of the step.
  Mutating while iterating EnTT views is unsafe, and deferring keeps the order deterministic.
- **Determinism habits:** id-sorted iteration where order matters (spatial-hash queries, contact
  resolution, projectile hits), floats only, fixed step order.

### Component catalogue

| Component | Purpose |
|---|---|
| `Transform`, `PrevTransform` | position (m) + rotation (rad, 0 = +Y); previous step snapshot for interpolation |
| `Velocity` | m/s |
| `ShipControl` | per-step thrust/turn intent (written by input; AI/net later) |
| `ShipStats` | acceleration, maxSpeed, turnSpeed, drag |
| `PlayerControlled` | tag: driven by local input |
| `SpriteRender` | albedo + normal `TextureHandle`, size (m), frameCount/frame |
| `Hitbox` | Circle / AABB / convex Polygon (≤ 8 verts), offset |
| `CollisionLayer` | `layer` + `mask` bitmasks (Ship, Wall, Projectile) |
| `StaticTag`, `Wall`, `Health` | immovable; wall marker + destructible flag; hit points |
| `Altitude` | pseudo-3D z, vz, falling, visualScale |
| `Light` | point/directional light with height, range, color, intensity |
| `Projectile`, `Lifetime`, `Range`, `Damage`, `Owner` | bullet tag; optional time limit; travel-distance limit; per-hit damage; shooter + team |
| `Bounce`, `SurfaceResponse` | projectile ricochets (bounces left, −1 = unlimited); wall is Absorbent or Reflective |
| `ForceField`, `ForceResponse`, `Deflector` | field source; Rigid/Deflectable; ship deflector ability |
| `AbilityLoadout` (in `gameplay/Abilities.hpp`), `BeamRequest` | equipped weapon slots; per-step beam intent |

## 6. Frame flow (`App::run`)

```
poll SDL events ─► ImGui; quit on close
timing:   accumulator += min(frameTime, 0.25 s)          (clamp prevents the spiral of death)
input:    keyboard ─► InputActions {thrust, turn, fire1, fire2}   (skipped if ImGui owns keyboard)

while accumulator >= 1/60:                                ── FIXED-STEP SIMULATION ──
    snapshot_prev_transforms      Transform → PrevTransform
    input_apply                   InputActions → ShipControl of PlayerControlled entities
    fire_step                     cooldowns + modifiers → queue ProjectileSpawns / set BeamRequest
    apply_fall_control            zero thrust while falling (if the flag says so)
    movement_step                 ships: turn, thrust, drag, clamp, integrate
    projectile_move_step          bullets: straight-line integrate
    forcefield_step               push Deflectable entities (wind, active deflectors)
    collision_step                ships push-out + slide vs walls (projectiles skipped)
    projectile_hit_step           swept raycast prev→cur per bullet → damage, ricochet or despawn
    beam_step                     hitscan beams (bouncing polyline) → damage/sec + beam viz
    floor_altitude_step           void gravity / recover / death+respawn
    lifetime_step                 expire timed entities
    range_step                    expire entities that used up their travel range
    commit_deferred               the ONLY place entities are created/destroyed
    accumulator -= 1/60
alpha = accumulator / (1/60)

camera:   elastic follow toward the ship's *interpolated* position (real frame dt)
ImGui:    Ship / Lighting / Camera panels (read + tweak live components)
queue:    background (unlit) → floor rects → render_collect(alpha) → debug ray → beams
GPU:      acquire cmd + swapchain
          renderer.upload + ImGui PrepareDrawData     ← all copies BEFORE the render pass
          BeginRenderPass(clear) → renderer.render(viewProj, lights) → ImGui → End
          submit
```

Rules baked into this order:
- `snapshot_prev_transforms` runs **inside** the loop, before movement, so rendering lerps from the
  pre-step pose to the collision-corrected pose (never into a wall).
- Collision runs **after** integration (move, then push out). Force fields run **before** collision
  so pushes are resolved against walls the same step.
- **Presentation never runs in the sim loop.** Lights are collected, the camera follows and sprites
  are queued at render time, with the real frame dt.
- Interpolation is a **plain lerp** on rotation (not shortest-arc), kept for behavioral identity
  since M1.

### Startup / shutdown
`init`: SDL_Init → window → GPU device (offers DXIL + SPIR-V; D3D12 on Windows, Vulkan on Linux) → claim window → ImGui (SDL3 + SDL_GPU backends)
→ Renderer → load ship defs → textures → wall defs + level → spawn walls, player (+ loadout),
lights, force fields, debug light → camera. Each step logs and bails out on failure.
`shutdown`: wait for GPU idle → textures → renderer → ImGui → release window → device → window →
SDL_Quit (reverse order, idempotent).

## 7. Cross-cutting conventions

- **Namespace** `sc`. Files `PascalCase.hpp/.cpp`; functions/variables `snake_case` for free
  functions (`movement_step`), `camelCase` for members/locals; members of classes prefixed `m_`.
- **Formatting:** `.clang-format` (LLVM base, 4-space indent, 100 columns), applied on save.
  `.editorconfig` for whitespace/line endings.
- **Units:** meters, seconds, radians. Rotation 0 faces **+Y**; forward = `(-sin θ, cos θ)`.
  World +Y is up.
- **Logging:** `SC_LOG_INFO/WARN/ERROR` macros in `core/Log.hpp` (header-only over `SDL_LogMessage`
  with an app category). Macros, not direct SDL calls, so the format/routing can change in one place.
- **Input:** abstract `InputActions`. Only `App` maps keys to actions (the seam for gamepad/touch).
- **Third-party code** never gets our strict warnings. `stb_image_impl.cpp` is compiled with `/w`.
- **Comments** explain *why*, not what; reference the brief section (`brief §6.4`) where relevant.

## 8. Build & tests

- **One executable target** `SpaceChase` (all `src/`) plus **`SpaceChase_tests`**, which compiles
  the GPU-free sources it exercises directly (no library split yet; deferred until editor/net give a
  second real consumer).
- Every new `.cpp` must be added to `CMakeLists.txt` (and to the test target if a test uses it).
- Tests live in `tests/test_*.cpp` (doctest; `test_main.cpp` holds the runner). Each milestone
  sub-step adds tests for its pure logic. Currently 73 cases. Run
  `build/windows-clang-cl/bin/Debug/SpaceChase_tests.exe`.
- Strict warnings + warnings-as-errors on our targets only (`cmake/warnings.cmake`). The test target
  skips `-Werror` because doctest's header isn't built to that policy.

## 9. Workflow conventions

- Work is planned per milestone (see [`../plans/`](../plans/)), built in **sub-steps that each
  compile and pass tests**, and **committed per sub-step**.
- All project documentation lives in `docs/`. New plans go to `docs/plans/`; when a module changes
  meaningfully, update its doc in `docs/technical/` in the same change.
