# Space Chase — Codebase Guide (Milestone 2 delta)

> Builds on `codebase-guide-m1.md`. The M0/M1 fundamentals — build system, the GPU/
> sprite-batch rendering path, the camera, the fixed-timestep rationale — are unchanged;
> see that doc. This covers what **M2 (ECS + data spine)** added and how the frame flow
> changed. Updated at the end of M2.

---

## 1. What M2 changed

The ship stopped being a plain `ShipState`/`ShipConfig` struct updated by free functions.
It is now an **EnTT entity** composed of POD **components**, advanced by an explicit ordered
**system pipeline** inside the same fixed timestep, and its stats are **loaded from a JSON
definition** through a typed `data/` registry. A **doctest** target covers the loader and the
movement step. Behavior is identical to M1 (same math, same literals — now in data).

New dependencies: **EnTT** (header-only ECS), **nlohmann/json** (definitions), **doctest** (tests).

---

## 2. Updated layer hierarchy

```
            game/ ─────────────► core/App  (composition root, owns World + DefinitionRegistry)
                                   │
   ┌──────────────┬───────────────┼───────────────┬───────────────┐
   ▼              ▼               ▼               ▼               ▼
 systems/      gameplay/        data/           ecs/          render/
 (logic over   ShipFactory   DefinitionRegistry  World        Renderer
  the registry) (spawn)       + ShipDef          + Registry    Camera
   │              │               │               │               │
   └──────────────┴───────► components/ (POD) ◄────┘               │
                                   │                                │
                                   ▼                                ▼
              glm / EnTT (entity model) ............ SDL3 / stb / ImGui
```

Dependency direction still flows down. `systems/` may touch `render/` (render-collect) and
`core/` (input); `components/` depend only on glm (and the texture header). `App` is still the
only place the sibling layers meet.

---

## 3. New files (M2)

### `src/components/` — POD data, one struct per header
`Transform` (position + rotation), `PrevTransform` (last-step snapshot, for interpolation),
`Velocity`, `ShipControl` (per-entity thrust/turn intent), `ShipStats` (accel/maxSpeed/turnSpeed/
drag, from data), `SpriteRender` (texture ptr + frame info), `PlayerControlled` (tag). No logic.

### `src/ecs/`
- `Registry.hpp` — the single EnTT include seam: `sc::Registry` = `entt::registry`, `sc::Entity`.
- `World.hpp` — `struct World { Registry registry; Entity playerShip; }`, owned by `App`.

### `src/systems/` — logic, called in a fixed order; signatures take `Registry&`
- `InputSystem` (`input_apply`) — copies the local `InputActions` into every `PlayerControlled`
  entity's `ShipControl`. The one place raw input meets the ECS.
- `MovementSystem` (`snapshot_prev_transforms`, `movement_step`) — the verbatim M1 momentum/drag
  integration; the snapshot copies `Transform`→`PrevTransform` at each step's start.
- `RenderCollectSystem` (`render_collect`) — interpolates `PrevTransform`→`Transform` by `alpha`,
  selects the tilt frame from `ShipControl.turn`, and queues each sprite into the batch.

### `src/data/` — the content spine
- `ShipDef.hpp` — POD `ShipDef` (id, displayName, spriteSheet, frameCount, frameSize, stats).
  JSON-free so the header stays light.
- `DefinitionRegistry.{hpp,cpp}` — loads every `*.json` under a directory into a typed map
  (`load_ships_from_dir`, `find_ship`). Bad files are logged and skipped; the parse lives here so
  nlohmann/json stays out of headers. This is the source of truth the editor will enumerate later.

### `src/gameplay/`
- `ShipFactory` (`spawn_ship`) — composes a ship entity from a `ShipDef` + texture (emplaces all
  components; `Transform == PrevTransform` initially). The data→entity step projectiles reuse in M5.
- `Ship.{hpp,cpp}` — **removed** (math moved into `MovementSystem`).

### `assets/definitions/ships/player_ship.json`
The player ship's data (stats = the exact M1 literals). Editing it changes handling with no rebuild.

### `tests/`
`test_main.cpp` (doctest entry), `test_definitions.cpp` (the shipped def parses; lookups behave;
missing dir fails gracefully), `test_movement.cpp` (thrust+drag integration, max-speed clamp, turn).

---

## 4. Changes to existing files

- `core/App.{hpp,cpp}` — members `m_ship/m_shipPrev/m_shipConfig` replaced by `World m_world` +
  `DefinitionRegistry m_defs`. `init()` loads definitions, then loads the texture from the def's
  `spriteSheet` and spawns the player via `spawn_ship`. `run()`'s sim/queue region is now the system
  pipeline (below). The "Ship" panel reads/edits the player entity's components.
- `CMakeLists.txt` — EnTT + nlohmann_json deps and links; new sources; a `SpaceChase_tests` target
  + `enable_testing()`/`add_test`. `CMakePresets.json` — added `testPresets`.

---

## 5. Updated per-frame flow (`App::run`)

Only the simulation/queue region changed from M1; events, timing, key sampling, and the GPU
acquire/upload/render-pass ordering are identical.

```
poll events → feed ImGui / quit
timing: accumulator += clamped frame time
sample keys → InputActions actions          (skipped if ImGui wants keyboard)

input_apply(registry, actions)               // intent → ShipControl, ONCE per frame
while accumulator >= 1/60:
    snapshot_prev_transforms(registry)        // Transform → PrevTransform
    movement_step(registry, 1/60)             // integrate all ships
    accumulator -= 1/60
alpha = accumulator / (1/60)

ImGui "Ship" panel (reads/edits player components)
renderer.begin()
render_collect(registry, renderer, alpha)     // per-entity lerp + tilt frame + draw

acquire cmd + swapchain
  renderer.upload(cmd); ImGui PrepareDrawData(cmd)   // uploads BEFORE the pass
  BeginRenderPass(clear) → renderer.render → ImGui RenderDrawData → EndRenderPass
submit
```

**Interpolation model:** `snapshot_prev_transforms` runs *inside* the step loop (before each
`movement_step`), mirroring M1's `m_shipPrev = m_ship`. The render lerps `PrevTransform`→`Transform`
by `alpha` — a **plain** lerp on rotation (not shortest-arc), matching M1 exactly. Spawn sets both
transforms equal so the first frame's lerp is a no-op.

---

## 6. The data pipeline (JSON → entity)

```
player_ship.json
   │  DefinitionRegistry::load_ships_from_dir   (nlohmann/json parse, log-and-skip)
   ▼
ShipDef (typed, in the registry)
   │  find_ship("player_ship")  +  ShipFactory::spawn_ship
   ▼
entity { Transform, PrevTransform, Velocity, ShipControl, ShipStats(from def), SpriteRender, PlayerControlled }
```

This is the "one source of truth" the runtime spawns from and the editor will enumerate (brief
§3.3/§7). Sibling definition kinds (abilities, projectiles…) slot into `DefinitionRegistry` the
same way in later milestones.

---

## 7. Why this shape (design notes)

- **POD components + systems-hold-logic** keeps EnTT's role minimal and the data replication-ready
  (brief §8). No system scheduler abstraction — the "pipeline" is literally the ordered calls in
  `run()`; that's enough for M2 and avoids over-engineering (brief §3.2).
- **`ShipControl` as a component** (rather than passing `InputActions` into the movement system)
  keeps every system's signature uniform (`Registry&`) and lets AI/network writers target the same
  component later without touching movement.
- **`World` owns the registry by value inside `App`** — not a global — so the sim stays
  self-contained.

---

## 8. M2 simplifications & what M3 changes

| M2 (now) | M3 (next, per brief §9) |
|---|---|
| `SpriteRender` holds a raw `const Texture*` into an App-owned texture | resource/texture **handle registry** (many sprites) |
| One ship entity | collision: **spatial hash**, hitboxes, raycast; many entities |
| No `Health`/`Hitbox` components | added for destructible walls / damage |
| Definitions loaded once | hot-reload of definitions becomes worthwhile |
| Single executable + test target | (still single target; lib split deferred) |
