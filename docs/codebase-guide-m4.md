# Space Chase — Codebase Guide (Milestone 4 delta)

> Builds on the M1–M3 guides. Covers what **M4** added: normal-mapped 2.5D
> lighting (point + directional lights with height, diffuse + specular + ambient,
> normals rotated with sprite rotation), plus the rendering changes it required.

---

## 1. What M4 changed

The sprite batch is now a **forward-lit, normal-mapped** pipeline:

- A new shader `assets/shaders/lit_sprite.hlsl` (hand-compiled to DXIL) samples an
  albedo + a normal map, **rotates the sampled normal by the sprite's rotation**
  (so lighting is world-consistent as ships turn — brief §6.7), and computes
  **diffuse N·L + Blinn-Phong specular + ambient** from up to 16 lights.
- Lights are **ECS entities** (`Transform` + `Light`), each with a **height above
  the plane** (the 2.5D knob). They're collected into a uniform buffer at **render
  time** (not the fixed-step sim).
- Every sprite now carries an optional **normal map**; sprites without one get a
  1×1 flat-normal fallback so the two-sampler pipeline is always satisfied.

Shadercross is still deferred (hand-compiled DXIL via `tools/compile_shaders.ps1`);
the HLSL is written to feed it later unchanged.

---

## 2. New files

- `assets/shaders/lit_sprite.hlsl` — the lighting shader (+ compiled `lit_sprite.{vert,frag}.dxil`).
- `src/render/LightsUBO.hpp` — GPU-facing `LightsUBO` / `GpuLight`, byte-matched to the cbuffer (`static_assert`). Lives in the render layer.
- `src/components/Light.hpp` — `Light` component (`LightType`, color, intensity, height, range, direction).
- `src/systems/LightingSystem.{hpp,cpp}` — `collect_lights(registry, ambient, params) -> LightsUBO` (render-time).
- `tools/gen_normals.ps1` — procedural bevel normal-map generator (run by hand).
- `assets/sprites/*_normal.png` — generated normal maps for ship/wall/crate.
- `tests/test_lights.cpp` — UBO packing / count / cap.

## 3. Changed files

- `render/Renderer.{hpp,cpp}` — vertex gains a `(cos,sin)` attribute; `draw()` takes an
  albedo **and** normal handle (overload defaults normal to the flat fallback); the batch
  flushes one draw per `(albedo, normal)` run binding **two** fragment samplers; `render()`
  pushes the camera matrix **and** the lights UBO; a 1×1 flat-normal texture is created in `init`.
- `components/SpriteRender.hpp` — gains a `normal` `TextureHandle`.
- `data/ShipDef`/`WallDef` + `DefinitionRegistry` — optional normal-map path; `LevelDef`/`LevelLoader`
  parse a `lights` array → `LevelData.lights`.
- `gameplay/ShipFactory`/`WallFactory` — take + set the normal handle.
- `systems/RenderCollectSystem.cpp` — passes the sprite's normal handle to `draw()`.
- `core/App.{hpp,cpp}` — loads normal textures; spawns level lights + a draggable debug light;
  builds the `LightsUBO` (`collect_lights`) and passes it to `render()`; adds a Lighting ImGui panel
  (ambient, shininess, specular, debug-light controls).

## 4. The lighting math (lit_sprite.hlsl)

- **Vertex:** forwards world position + the sprite's `(cos,sin)` to the fragment stage.
- **Fragment:** unpack normal `n = tex*2-1`; **rotate `n.xy` by the same `(c,s)` the CPU baked
  into the vertex positions**; `N = normalize(nr, n.z)`. View is straight down `V=(0,0,1)`.
  Per light: point → direction/distance to `(light.xy, height)` with range attenuation;
  directional → `normalize(dir, height)`. Accumulate `albedo*radiance*N·L` (diffuse) +
  `radiance*pow(N·H, shininess)*specStrength*N·L` (specular). Add `albedo*ambient`.
- **Height is the knob:** low = grazing/dramatic, high = flat wash.

## 5. Conventions / gotchas locked in

- **Normal rotation uses the SAME `(c,s)`** as the position baking — not the inverse. Verify
  interactively: a fixed world light keeps lighting the same world-facing side as the ship spins.
- **Green-channel convention** documented in `gen_normals.ps1` (one place to flip if inverted).
- **Lights are render-time**, never inside the fixed-step accumulator loop (presentation, not sim).
- **`LightsUBO` byte layout** must match the HLSL cbuffer (`static_assert` in the header).
- **Two samplers always bound** (flat-normal fallback) — required by the pipeline.

## 5b. Post-M4 fixes

- **Void is unlit.** The lit shader has a per-vertex `emissive` flag (`Renderer::draw(..., unlit)`);
  when set it returns the albedo untouched by lights. The void/background uses it.
- **Void is one background, not squares.** `Level` dropped `voidRects` — floor is now a union of
  floor rects and **void = the gaps**. App draws a single **unlit, camera-locked background quad**
  (`background.png`) behind everything; the floor rects (lit) draw on top and pits reveal the
  backdrop. (Parallax-ready: offset the backdrop by `camera * factor` later.)
- **Elastic camera follow.** `render/CameraFollow.hpp` defines a swappable `CameraFollow` interface
  with two models — `SmoothDampFollow` (critically damped, no overshoot) and `ExponentialFollow`
  (exponential lag). App holds one (`std::unique_ptr`), targets the ship's **interpolated** position
  each frame with the real frame dt, and **translates only** (never rotates). Switchable + tunable in
  the Camera ImGui panel.

## 6. What M5 changes (next, per brief §9)

- **Abilities + projectiles**: the data-driven firing pipeline, projectile lifecycle, a few ability
  types + modifiers, and the unified **force-field** system (the `physics/ForceApi` stub gets a real
  implementation). Spawns many short-lived entities — the spatial hash's first real stress test.
- Still pending as their own milestones: **SDL_shadercross** (cross-platform shaders), multi-floor /
  parallax (M6), the editor + full level format (M7).
