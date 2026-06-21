# Space Chase — Codebase Guide (through Milestone 1)

> A living, abstract-level map of the codebase: what each file does, how the layers
> depend on each other, and how a frame actually executes. Updated at the end of M1
> (render + move one ship). For the *why* behind the architecture, see
> `sdl3-spaceshooter-groundwork-brief.md`.

---

## 1. What exists so far

- **M0 (bootstrap):** SDL3 window + GPU device, a render-pass clear, Dear ImGui wired in,
  logging, and the CMake/FetchContent build.
- **M1 (this milestone):** a 2D **sprite batch** on the SDL_GPU API, an orthographic
  **camera**, a **fixed-timestep simulation** decoupled from rendering, **action-based input**,
  and a **momentum/drag ship** with a procedural **tilt sprite sheet**.

The app today: a ship you fly with WASD/arrows around a dark field, with a live ImGui panel
to tune its handling.

---

## 2. Layer hierarchy & dependency direction

Dependencies flow **downward only** (brief §4: `game → engine layers → core`). Nothing in a
lower layer includes a higher one.

```
                         ┌─────────────┐
                         │  game/      │  entry point, owns the App
                         │  main.cpp   │
                         └──────┬──────┘
                                │
                         ┌──────▼──────┐
                         │  core/      │  App (the loop), Log, Input
                         │  App        │
                         └──┬───┬───┬──┘
              ┌─────────────┘   │   └─────────────┐
        ┌─────▼─────┐    ┌──────▼──────┐   ┌───────▼────────┐
        │ render/   │    │ gameplay/   │   │ resources/     │
        │ Renderer  │    │ Ship        │   │ Texture        │
        │ Camera    │    └─────────────┘   └────────────────┘
        └─────┬─────┘
              │
   ┌──────────▼───────────────────────────────────────────┐
   │  Third-party: SDL3 (platform/GPU), Dear ImGui (UI),   │
   │  glm (math), stb_image (decode)                        │
   └───────────────────────────────────────────────────────┘
```

`core/App` is the composition root: it owns the window/device and the higher-level objects
(`Renderer`, `Texture`, `Ship*`, `Camera2D`) and wires them together each frame. The sibling
layers (`render`, `gameplay`, `resources`) don't know about each other — `App` is the only
place they meet. This keeps the layers swappable and is what the M2 ECS refactor will slot into.

---

## 3. File-by-file (abstract)

### Entry point
| File | Role |
|---|---|
| `src/game/main.cpp` | Process entry. Includes SDL's `<SDL3/SDL_main.h>` shim once, constructs `sc::App`, runs `init → run → shutdown`, returns non-zero if init fails. Deliberately tiny. |

### Core (`src/core/`)
| File | Role |
|---|---|
| `App.hpp/.cpp` | The application shell and **the main loop**. Owns the SDL window, GPU device, ImGui context, the `Renderer`, the ship's sprite `Texture`, the `Camera2D`, and the ship simulation state. `init()` brings subsystems up in dependency order; `run()` is the per-frame loop (§5); `shutdown()` tears everything down in reverse. |
| `Log.hpp` | Header-only logging: `SC_LOG_INFO/WARN/ERROR/...` macros over SDL's categorized logging. No external dep; a seam so logging behavior can change in one place later. |
| `Input.hpp` | `InputActions { thrust, turn }` — **abstract** inputs decoupled from raw keys. The layer that makes a future touch/gamepad port cheap: only the key→action mapping changes, not gameplay. |

### Render (`src/render/`)
| File | Role |
|---|---|
| `Renderer.hpp/.cpp` | The **sprite batch**. Owns the graphics pipeline, sampler, a dynamic vertex buffer, and a staging buffer. Per frame: `begin()` clears the queue, `draw(...)` appends a world-space quad (with UV sub-rect for sheet frames), `upload(cmd)` pushes vertices to the GPU (outside a render pass), `render(cmd, pass, viewProj)` issues one draw call. M1 = one texture per batch. |
| `Camera.hpp` | Header-only `Camera2D`: position + zoom → a world-to-clip orthographic matrix (`glm::ortho`, +Y up). 1 world unit = 1 pixel at zoom 1. |

### Resources (`src/resources/`)
| File | Role |
|---|---|
| `Texture.hpp/.cpp` | `Texture { handle, width, height }` plus `load_texture()` (decode an image via stb_image → create + upload an SDL_GPU sampler texture through a transfer buffer + copy pass) and `destroy_texture()`. |
| `stb_image_impl.cpp` | The single translation unit that compiles stb_image's implementation (`STB_IMAGE_IMPLEMENTATION`). Isolated so its third-party warnings can be silenced (CMake compiles just this file with `/w`) while our code keeps `-Werror`. |

### Gameplay (`src/gameplay/`)
| File | Role |
|---|---|
| `Ship.hpp/.cpp` | The ship's data + physics. `ShipConfig` (accel/maxSpeed/turnSpeed/drag — tunable, data-driven later), `ShipState` (position/velocity/facing), `ship_step()` (one fixed-step of the momentum/drag model), `ship_lerp()` (interpolate two states for smooth rendering). A plain struct now; becomes ECS components + a movement system in M2. |

### Assets & tools
| File | Role |
|---|---|
| `assets/shaders/sprite.hlsl` | The sprite shader, authored once in HLSL. Vertex stage applies the camera matrix; fragment samples the texture. Documents SDL_GPU's register-space binding rules. |
| `assets/shaders/compiled/sprite.{vert,frag}.dxil` | DXIL bytecode (committed). Loaded at runtime by the `Renderer`. Regenerated by the tool below. |
| `assets/sprites/ship_sheet.png` | Procedural 3-frame tilt sheet (bank-left / neutral / bank-right), 192×64. |
| `tools/compile_shaders.ps1` | Compiles the HLSL → DXIL with the Windows SDK `dxc`. The "staged" stand-in for the future SDL_shadercross build step (which will also emit SPIR-V/MSL). |

### Build
| File | Role |
|---|---|
| `CMakeLists.txt` | Single root build script. FetchContent pins (SDL3, ImGui, glm, stb), the `SpaceChase` target + its sources, the `SC_ASSET_DIR` define (dev loads assets from the source tree), and the per-file warning exemption for stb. |
| `cmake/imgui.cmake` | Builds the `imgui` static lib from fetched sources (core + SDL3 + SDL_GPU backends). |
| `cmake/warnings.cmake` | `spacechase_set_warnings()` — strict warnings + `-Werror` on our targets only, per compiler front-end. |
| `CMakePresets.json` | Toolchain presets: `windows-clang-cl` (default), `windows-msvc`, Linux/macOS stubs; Ninja Multi-Config. |

---

## 4. New code in existing (M0) files

| File | What M1 added |
|---|---|
| `core/App.hpp` | Includes for `Ship`, `Camera`, `Renderer`, `Texture`; new members: `Renderer m_renderer`, `Texture m_shipTexture`, `Camera2D m_camera`, and `ShipState m_ship` / `m_shipPrev` / `ShipConfig m_shipConfig`. |
| `core/App.cpp` | (a) device now requests **DXIL only** → deterministic D3D12 backend matching our shaders; (b) `init()` now also initializes the renderer and loads the ship sheet; (c) `run()` was rewritten from "clear + ImGui" into the full **fixed-timestep loop** (timing → input → sim steps → interpolate → ImGui panel → queue batch → upload-then-draw); (d) `shutdown()` releases the texture + renderer before the device. |
| `CMakeLists.txt` | Added glm + stb FetchContent and the `stb` interface target; added the new `render`/`resources`/`gameplay` sources; added `SC_ASSET_DIR` and `GLM_FORCE_DEPTH_ZERO_TO_ONE` defines; per-file `/w` for the stb impl TU. |

---

## 5. Execution flow

### Startup (`App::init`)
Strict dependency order, each step bailing out (logged) on failure:
```
SDL_Init → CreateWindow → CreateGPUDevice(DXIL) → ClaimWindowForGPUDevice
        → ImGui (context + SDL3 backend + SDL_GPU backend)
        → Renderer.init(swapchain format)        // pipeline, sampler, buffers
        → load_texture(ship_sheet.png)           // decode + GPU upload
```

### Per-frame loop (`App::run`)
```
┌─ poll SDL events ───────────────► feed ImGui; handle quit/close
│
├─ TIMING:   accumulator += clamped frame time
│
├─ INPUT:    sample keyboard → InputActions   (skipped if ImGui wants keyboard)
│
├─ SIMULATE: while accumulator >= 1/60s:
│               m_shipPrev = m_ship
│               ship_step(m_ship, cfg, actions, 1/60)   // fixed steps
│               accumulator -= 1/60
│            alpha = accumulator / (1/60)
│            rendered = ship_lerp(m_shipPrev, m_ship, alpha)   // smooth
│
├─ UI:       ImGui new frame → "Ship" tuning panel → ImGui::Render
│
├─ QUEUE:    renderer.begin()
│            pick tilt frame from actions.turn → UV sub-rect
│            renderer.draw(shipTexture, rendered.pos, 64x64, rendered.facing, uv)
│
└─ GPU:      acquire command buffer + swapchain texture
                renderer.upload(cmd)              ─┐  uploads BEFORE
                ImGui PrepareDrawData(cmd)         ─┘  the render pass
                BeginRenderPass (clear)
                   renderer.render(cmd, pass, viewProj)   // world, under UI
                   ImGui RenderDrawData(cmd, pass)        // UI on top
                EndRenderPass
             SubmitCommandBuffer
```

The ordering constraint that shapes this: **all GPU copy operations (vertex upload, ImGui's
draw-data upload) must happen before any render pass begins** — SDL_GPU forbids copies during an
active render pass. So both uploads sit ahead of `BeginRenderPass`.

### Shutdown (`App::shutdown`)
`WaitForGPUIdle → destroy texture → renderer.shutdown → ImGui shutdown → release window from
device → destroy device → destroy window → SDL_Quit`. Reverse of init; idempotent.

---

## 6. The fixed-timestep model (why it's shaped this way)

Simulation advances in fixed 1/60 s steps; rendering interpolates. An `accumulator` banks
real elapsed time and is drained one fixed step at a time, so physics is **frame-rate
independent** (same behavior at 30 or 144 fps) and stable. Because a frame usually lands
*between* sim steps, we keep the previous state and render `lerp(prev, current, alpha)` where
`alpha` is the leftover fraction — this removes stutter. This split is also the prerequisite for
clean netcode later (brief §3.4/§8). The frame-time clamp (0.25 s) prevents a "spiral of death"
if the app stalls.

---

## 7. Rendering path (CPU → GPU)

1. **Texture upload (once):** stb decodes PNG → RGBA bytes → copied into a transfer buffer →
   `SDL_UploadToGPUTexture` in a copy pass → a sampler texture.
2. **Per-frame batch:** `draw()` computes each sprite's four world-space corners on the CPU
   (rotation + scale + translate) with UVs from the sheet sub-rect, appending 6 vertices.
   `upload()` copies the whole vertex array to the GPU once.
3. **Draw:** bind pipeline + vertex buffer + texture/sampler, push the camera `viewProj` as a
   vertex uniform, one `DrawGPUPrimitives` for all queued sprites.
4. **Shader binding model (must match exactly):** SDL_GPU maps HLSL resources by register space
   — vertex uniforms in `space1`, fragment texture/sampler in `space2`; and vertex **inputs** by
   `TEXCOORD<n>` where `n` = attribute location. (Both were M1 bug sources; now encoded in
   `sprite.hlsl` and `Renderer.cpp`.)

---

## 8. Runtime objects at a glance

| Type | Lives in | Holds | Mutated by |
|---|---|---|---|
| `App` | stack (`main`) | window, device, ImGui, renderer, texture, camera, ship | the loop |
| `ShipState` | `App` | position, velocity, facing | `ship_step` (sim) |
| `ShipConfig` | `App` | handling tunables | ImGui sliders |
| `InputActions` | loop-local | thrust, turn | keyboard sampling |
| `Camera2D` | `App` | position, zoom | (static in M1) |
| `Renderer` | `App` | pipeline, sampler, buffers, vertex queue | per-frame draw calls |
| `Texture` | `App` | GPU texture handle + size | load/destroy |

---

## 9. M1 simplifications & what M2 changes

| M1 (now) | M2 (next) |
|---|---|
| Plain `ShipState`/`Ship` struct | EnTT components + a movement **system** |
| Single-texture sprite batch | multi-texture / atlas batching for projectiles |
| DXIL-only, by-hand shader compile | SDL_shadercross **build step** → SPIR-V/DXIL/MSL |
| Ship handling as code literals | ship stats loaded from a **definition file** (data-driven) |
| Static camera at origin | (camera follow / world bounds as needed) |
