# SDL3 Top-Down Space Shooter — Groundwork & Architecture Brief

> **Purpose of this document.** This is an input brief for Claude Code *plan mode*. It defines the
> intended tech stack, architecture, repository layout, and build order for the project, and flags
> the open decisions to resolve interactively. The emphasis is **groundwork and structure for
> long-term maintainability without bloat** — not implementation of features yet. Where a decision
> is opinionated, an alternative is noted so it can be challenged during planning.

---

## 1. Project summary

A 2D, sprite-based, top-down space-ship shooter, built directly on **SDL3** (not an engine).
Cross-platform desktop first (Linux / macOS / Windows) with cross-play, architected so a **mobile
port (Android/iOS) stays cheap** and so **multiplayer (ENet)** can be added later without a rewrite.
Rendering is 2D sprites with **dynamic normal-mapped lighting** ("2.5D lighting": 3D lighting math on
flat geometry). The game must support a very large, ever-growing variety of ships, abilities, and
environment pieces, plus a **level editor usable by both developers and players**.

The two hardest design forces, which shape everything below:
1. **Ability/content variety is effectively unbounded** → the engine must be *data-driven* and use
   *composition* (ECS), never class hierarchies of weapons/ships.
2. **The level editor and the runtime must share one source of truth** → entity types, assets, and
   the level format are defined once and consumed by both.

---

## 2. Tech stack & key decisions

| Concern | Recommendation | Alternative (flag for planning) |
|---|---|---|
| Language | **C++20** | Rust (sdl3 crate + hecs); safer/cleaner but steeper, less mature SDL3 bindings |
| Platform / window / input / audio | **SDL3** | — |
| GPU / rendering | **SDL3 GPU API** (Vulkan/Metal/D3D12) | SDL3 2D renderer (no custom shaders — rejected: we need shaders) |
| Shaders | author once, cross-compile via **SDL_shadercross** | — |
| ECS | **EnTT** (header-only, lean) | flecs (more batteries: reflection/relationships, helps editor; heavier) |
| Math | **glm** (header-only) | minimal hand-rolled vec/mat |
| Tooling / debug / editor UI | **Dear ImGui** (SDL3 + SDL_GPU backends) | — |
| Networking (later) | **ENet** | GameNetworkingSockets (heavier, encrypted) |
| Data / definition format | **JSON** (editor-friendly, human-diffable) | TOML (toml++), or custom binary for shipped builds |
| Tests | **doctest** | Catch2 |
| Build | **CMake + FetchContent** | vcpkg / Conan |

Keep the dependency set roughly to the list above. Every added dependency should be justified
against "could a small amount of our own code do this instead?" — that is the anti-bloat test.

---

## 3. Architectural principles (load-bearing)

These are the rules the rest of the design must not violate:

1. **Data-driven content.** Ships, abilities, modifiers, projectiles, environment pieces, and levels
   are *data files* interpreted by generic systems — not hardcoded classes. Adding a new weapon
   should usually mean adding a definition file plus, at most, a small composable component, never a
   new subclass.
   *Why this is non-negotiable here, not stylistic:* the player-facing level editor implies content
   must live in data that the editor and runtime both read — if abilities were code, players couldn't
   create them and the editor couldn't enumerate them. So data-driven is implied by a feature already
   on the list, not a preference. Secondary wins that come for free: tuning ship/ability params without
   recompiling (fast iteration), and community/mod content as a multiplier if the game finds a
   following.

   (This is incorrect. For abilities we dont want it to be modifiable by users, but ofc, the same design principles would still be nice from a development pov I guess.)
2. **Composition over inheritance.** Behavior is assembled from small, mixable components — not class
   trees.
   *Why this is non-negotiable here:* the ability design has several *orthogonal* axes that combine
   freely — projectile type (bullet / missile / mine / ray / chi) × homing × fire cadence × stream
   count × … . Under inheritance, every combination wants to be its own class
   (`HeatseekingMultifireMissile`), so the class count grows *multiplicatively* with the number of
   axes — the classic combinatorial subclass explosion. Composition turns that multiplication into
   addition: one component per axis, mixed at will. This is precisely the case inheritance handles
   worst, which is what makes it load-bearing rather than a slogan.
   *How far to take it (and what to challenge):* hold the *principle* (composition + data-driven)
   firm. Treat the specific **ECS framework** (EnTT) as the *negotiable* part — it buys composition
   plus good performance over many entities, but it adds indirection and less linear debugging than a
   straight call stack. Lighter structures (has-a components, struct-of-arrays) can deliver most of
   the benefit without a full ECS; if a formal ECS starts to feel heavier than this game needs,
   push back on it in planning. Note also that all of this emphasis is *conditional on the scope
   above* (unbounded ability variety + combining modifiers + a player editor) — a small, fixed game
   would not warrant it.
3. **One source of truth for content.** The editor and the runtime load the *same* definition
   registries and the *same* level format. The editor is a thin authoring layer over runtime data.
4. **Fixed-timestep simulation, decoupled from rendering.** Simulation runs at a fixed rate;
   rendering interpolates. This stabilizes movement/physics and is a prerequisite for clean netcode.
5. **Soft engine/game split.** Reusable layers (core, render, ecs, physics, resources) are kept
   separable from game-specific layers (gameplay, game, editor) — but we are *not* building a general
   engine product. Only generalize what the game actually needs.
6. **Unify similar mechanics.** Distinct-looking features that are the same underneath share one
   system (see: wind zones, deflectors, and chi-push are all **force fields**).
7. **Hot-reload in dev.** Assets and definitions reload at runtime for fast iteration.
8. **Multiplayer-shaped from day one, implemented late.** Keep sim/render split, keep entity state
   serializable/replicable, keep the simulation as self-contained as possible — but build
   single-player first.

---

## 4. Repository structure

```
/ (repo root)
  CMakeLists.txt              # FetchContent for SDL3, EnTT, glm, imgui, enet, json, doctest
  /cmake                      # build helpers, shader-compile step (shadercross)
  /external                   # third-party (submodule or FetchContent cache)
  /assets
    /shaders                  # shader source (authored once); compiled at build via shadercross
    /sprites                  # sprite sheets + normal maps
    /audio
    /definitions              # GAME CONTENT AS DATA: ships, abilities, modifiers, env pieces
    /levels                   # level files (same format the editor writes)
  /src
    /core                     # app loop, window, GPU device, time, logging, types, fixed-timestep
    /math                     # glm wrappers / helpers
    /render                   # sprite batch, lighting pipeline, camera, particles, layer/parallax
    /ecs                      # registry setup, component registration, system scheduler
    /components               # POD component structs
    /systems                  # systems: movement, firing, collision, forcefield, portal, floor...
    /physics                  # integration, spatial hash, hitbox tests, raycast, force application
    /resources                # asset loading, handles, hot-reload
    /data                     # definition loading + serialization + registries (the data-driven core)
    /gameplay                 # ships, projectiles, abilities, environment, portals, floor-layers
    /net                      # networking (stubbed initially, ENet later)
    /editor                   # level editor (ImGui) — consumes /data registries + /levels format
    /ui                       # HUD, menus
    /game                     # scene/state glue, entry point
  /tools                      # standalone tools (asset packer, etc.)
  /tests                      # doctest unit tests
  /docs                       # this brief, conventions, design notes
```

The dependency direction should flow **game → engine layers → core**, never the reverse. `editor`
and `net` depend on `data`/`gameplay`; nothing in the lower layers depends on them.

---

## 5. Core systems (engine groundwork)

- **App / platform (`core`)** — SDL3 init, window, GPU device, the main loop with **fixed-timestep
  update + interpolated render**, input polling/mapping (abstract actions, not raw keys — this is the
  layer that makes the mobile touch port cheap), timing, logging, assertions.
- **Rendering (`render`)** — sprite batch renderer on the SDL3 GPU API; **normal-mapped lighting
  pipeline** (point + directional lights, each with a height/z above the sprite plane; per-pixel N·L;
  **rotate sampled normals by sprite rotation** so lighting stays world-consistent as ships turn);
  camera; particle/trail rendering; **layered rendering with parallax** keyed off floor-z.
- **ECS (`ecs` / `components` / `systems`)** — EnTT registry; component registration; an explicit
  **system update order** (the fixed-step pipeline). Components are plain data; systems hold logic.
- **Physics / collision (`physics`)** — custom top-down 2D (no general physics engine). Provides:
  velocity integration with the momentum/drag model; **spatial hash broadphase** (sized for many
  projectiles); hitbox tests (circle / AABB, maybe convex); **raycasts** for beams and the
  machine-gun; **force application** API shared by force fields. *Flag determinism requirements vs.
  the chosen multiplayer model.*
- **Resources (`resources`)** — texture/normal-map/sprite-sheet/audio/definition loading, handle
  registry, dev **hot-reload**.
- **Data / definitions (`data`)** — load definition files into typed registries; serialization;
  the shared backbone for both runtime and editor. **Build this early — most content depends on it.**
- **Audio** — SDL3 audio + a small mixer/bus abstraction.

---

## 6. Gameplay architecture (the heart)

### 6.1 Entities as composed components
Ships, projectiles, environment pieces, pickups are all ECS entities composed from components such as:
`Transform`, `Velocity`, `ShipStats`, `Health`, `Hitbox`, `Sprite`, `NormalMap`, `Trail`, `Lifetime`,
`Owner`, `ForceAffected`, plus **behavior tags** (`Homing`, `Persistent`, `Deployable`, `RayWeapon`,
`Warp`, ...). New behavior = new component + small system, not a new class.

### 6.2 Ships
Defined entirely in data: `speed`, `acceleration`, `turnSpeed`, `momentum/drag` (slipperiness),
`health`, `bulletSpeed`, `damage`, ... (extensible). Visuals: a sprite sheet selected by **tilt state**
(forward/back/left/right, driven by acceleration/input), with **top-down rotation applied as a
transform**. Each ship has a `Hitbox`. Normal maps drive the dynamic lighting.

### 6.3 Abilities — the data-driven firing pipeline
This is the most important subsystem. Model it as a pipeline, not a type hierarchy:

```
AbilityDefinition (data)  +  active Modifiers (data)
        │
        ▼
   FireSystem  ──►  spawns projectile/effect entities by *composing components*
                   (movement variant, hitbox/ray variant, damage, lifetime, visual+trail, behavior tags)
```

- **Ability definition** describes: category, what it spawns/does, parameters, and visuals (projectile
  sprite + trail/smoke effect).
- **Modifiers transform the spawn** at fire time and are themselves data + tiny composable logic:
  `multifire` = spawn N with angular spread; `quickfire` = cooldown/projectile-speed multiplier;
  `autofire` = hold-to-fire input flag; `heatseeking` = attach `Homing` to spawned projectiles.
- Adding a new ability or modifier is mostly a new definition file plus, at most, one small component.

**Proposed ability taxonomy** (refine in planning):
- **Offensive**
  - *Projectile* — travels: bullets, missiles, rockets.
  - *Beam / Ray* — hitscan/continuous via raycast: **machine-gun** (modeled as a conical beam with a hitbox, not thousands of bullets). And the cone gets dynamically blocked. So part of it can be blocked (and hence hit something, but the rest still carries on. Idk if it makes more sense to then turn it into small angle discrete beams, each of which gets blocked fully if it hits something), Also, persistent laser-beam which is just a ray.
  - *Deployable* — placed & persistent, triggers on proximity/time: mines, laid lasers.
  - *Field / Warp* — area effect on the space it occupies/passes through: **chi** (space-warp
    distortion). *Open: visual-only screen distortion vs. also applying displacement force.*
- **Defensive**
  - *Shield* — HP buffer / absorb.
  - *Deflector* — pushes movable objects away with a **force that can be overcome** (a force field).
- **Modifier** (alters offensive abilities) — heatseeking, quickfire, autofire, multifire, ...
- **Utility / Movement** (future) — boosts, blinks, etc.

### 6.4 Force fields — one system, many features
**Wind zones (directional / radial / spiral), deflectors, and chi-push are all the same thing:** a
`ForceField` component (shape + force function) whose system applies force to entities with
`ForceAffected`. Because it's a force (not a hard constraint), it can be overcome — exactly the
deflector requirement. Build this once.

### 6.5 Environment model
- **Static geometry** (floor, indestructible walls): grid/tile-based per floor-layer for efficiency.
- **Dynamic pieces** (destructible walls, machinery): entities with `Health`; machinery carries an
  `ExplodeOnDeath` behavior.
- **Void / falling**: a per-cell, per-layer **floor-presence mask**. A ship over a cell with no floor
  at its current layer enters a falling state → either drops to a lower layer that *does* have floor
  there, or dies if none.
- **Portals**: paired trigger volumes; entering within bounds pulls the ship in and transports it to
  its linked portal.

### 6.6 Floor layers & pseudo-3D (ambitious — scope carefully)
- A **floor-layer (z-level)** abstraction: each layer has its own floor mask, walls, and entities.
- **Parallax**: non-active layers render offset for depth.
- **Falling between layers**: moving over void transitions the entity's z-level downward.
- **Pseudo-3D walls between floors**: feasible — render vertical wall "side" quads/sprites between
  adjacent layer heights to fake elevation. Make this a **toggle (with/without walls)** as requested.
- *Flag: decide whether multi-layer is a v1 feature or a later addition — it is the most complex
  environment feature and can be deferred behind a clean layer abstraction.*

### 6.7 Lighting (per earlier design)
Normal-mapped 2.5D: point + directional lights, each with a **height above the plane** (the key knob),
per-pixel N·L, **normals rotated with sprite transform**. Flat sprites let us skip full tangent-space
TBN setup. Shaders authored once, cross-compiled via shadercross.

---

## 7. Level format & editor

- Define the **level file format first** (likely the same JSON family as definitions): a list of
  placed entity instances referencing definition IDs, plus tile/floor-mask data per layer, force
  zones, portals, spawn points.
- The **editor (`editor`, Dear ImGui)** enumerates the **definition registries** to populate its
  asset palette automatically, lets the user place/configure instances, and **writes the same level
  format the runtime loads**. Because content is data-driven, the editor stays thin and new content
  appears in it for free.
- Same tool serves devs and (eventually) players; player-facing UX polish can come later.

---

## 8. Multiplayer considerations (architect now, build later)

Build single-machine first (so single player or split screen multiplayer), but keep the door open for networking:
- Keep simulation in the fixed-step pipeline, separate from rendering.
- Keep entity state serializable for replication; keep systems side-effect-contained.
- Plan toward an **authoritative-server** model; expect to add **client prediction + reconciliation**
  later (the transport — ENet — is the easy part; the prediction layer is the real work).
- *Flag: co-op vs. competitive PvP, and target player count, strongly affect how much determinism and
  prediction rigor is needed. Resolve early — it influences the physics/sim design.*

---

## 9. Suggested build order (milestones)

0. **Bootstrap** — CMake + deps, SDL3 window + GPU device, clear screen, ImGui + logging wired.
1. **Render + move one ship** — sprite batch, camera, sprite-sheet tilt animation, action-based input,
   the momentum/drag movement model.
2. **ECS + data spine** — solidify EnTT registry, component/system order, load ship stats from a
   definition file.
3. **Collision** — spatial hash, hitboxes, raycast; destructible/indestructible walls; floor-mask,
   void & falling (single layer).
4. **Lighting** — normal-mapped pipeline with light height + normal rotation.
5. **Abilities** — the firing pipeline + projectile lifecycle + a few ability types across categories
   + a couple of modifiers; the unified **force-field** system (wind + deflector).
6. **Environment depth** — portals, machinery/explosions, then multi-floor + parallax (+ optional
   pseudo-3D walls).
7. **Level format + editor** (ImGui), sharing the registries.
8. **Audio, HUD/UI.**
9. **Networking** (ENet) — implement against the already-multiplayer-shaped sim.

---

## 10. Open decisions to resolve in planning

1. **Language**: C++20 (recommended) vs. Rust.
2. **ECS library**: EnTT (lean) vs. flecs (reflection/relationships could help the data + editor) vs.
   custom.
3. **Data format**: JSON vs. TOML vs. custom binary for shipped builds.
4. **Multiplayer model & scope**: co-op vs. competitive PvP; player count; authoritative vs. P2P;
   how much determinism — this gates the physics/sim design.
5. **Floor-layers / z**: v1 feature or deferred behind a layer abstraction?
6. **Pseudo-3D walls**: in scope for v1?
7. **Chi / space-warp mechanics**: visual distortion only, or gameplay-affecting displacement force?
8. **Target projectile / entity counts**: sets how hard the spatial-hash/ECS performance work must be.
9. **Editor**: in-engine panel vs. separate executable; scope of the player-facing version.
10. **Units & timestep**: world units, fixed simulation rate (e.g., 60 Hz), interpolation policy.

## 11. Explicitly out of scope for v1 (keep it lean)
- General-purpose engine features the game doesn't use.
- Full netcode prediction/reconciliation (architect for it; implement after single-player is solid).
- Polished player-facing editor UX (functional dev editor first).
- Compute-shader particle systems / advanced multi-pass post-FX beyond the lighting + basic effects.
