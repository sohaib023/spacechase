# Rendering

GPU device, the lit sprite batch, textures, the lighting model, shaders and the camera.
Code: `src/render/`, `src/resources/`, `src/systems/RenderCollectSystem.*`,
`src/systems/LightingSystem.*`, `assets/shaders/`.

---

## 1. GPU backend

- SDL3 **GPU API**. `App::init` creates the device requesting **`SDL_GPU_SHADERFORMAT_DXIL` only**,
  which deterministically selects **D3D12** and matches our committed shader blobs. Debug mode is on.
- **Shaders are HLSL, hand-compiled to DXIL** by `tools/compile_shaders.ps1` (Windows SDK `dxc`,
  profiles `vs_6_0` / `ps_6_0`). The `.dxil` files in `assets/shaders/compiled/` are committed, so
  **after editing any `.hlsl` you must re-run the script**.
- **SDL_shadercross is deliberately deferred** to its own milestone. The HLSL is written to SDL_GPU
  conventions so shadercross can later consume it unchanged and emit SPIR-V (Vulkan) + MSL (Metal).
  Until then the game runs on Windows only.

### SDL_GPU binding conventions (must match exactly)

| Resource | HLSL register |
|---|---|
| Vertex-stage uniform buffers | `b<n>, space1` |
| Fragment textures / samplers | `t<n>, space2` / `s<n>, space2` |
| Fragment uniform buffers | `b<n>, space3` |
| Vertex **inputs** | semantic **`TEXCOORD<n>`**, where `n` = vertex attribute location |

Using `POSITION` for a vertex input makes D3D12 fail pipeline creation with `E_INVALIDARG`. This was
a real M1 bug.

## 2. The sprite batch (`render/Renderer`)

One **forward-lit, alpha-blended** pipeline draws everything (ships, walls, floor, bullets, the void
backdrop, debug viz).

Per frame:
1. `begin()` clears the queue.
2. `draw(albedo, normal, pos, size, rotation, uvMin, uvMax, unlit)` appends one quad = 6 vertices.
   The CPU **bakes rotation/scale/translation** into world-space (meter) positions. Each vertex is
   `{x, y, u, v, cos, sin, emissive}`. The sprite's `(cos, sin)` rides along so the fragment shader can
   rotate normals. `unlit = true` sets `emissive = 1`. A second overload omits the normal handle
   (flat normal).
3. `upload(cmd)` copies all vertices into the GPU vertex buffer via a persistent transfer buffer.
   **This must happen before any render pass begins**, because SDL_GPU forbids copy passes inside a render
   pass. (Same for ImGui's `PrepareDrawData`.)
4. `render(cmd, pass, viewProj, textures, lights)`: binds pipeline + VBO once, pushes the camera
   matrix (vertex uniform 0) and the `LightsUBO` (fragment uniform 0) once, then walks the queue and
   issues **one draw per consecutive run of the same `(albedo, normal)` pair**, binding two
   fragment samplers each time.

Details:
- Capacity `kMaxSprites = 4096` per frame. Overflow is silently dropped.
- Blend: straight alpha (`src_alpha, 1-src_alpha`). No depth test, so **draw order = queue order**.
  App queues: background → floor rects → entities (`render_collect`) → debug ray → beams.
  Same-texture sprites should be queued consecutively to keep runs long.
- Sampler: linear filtering, clamp-to-edge.
- **Flat-normal fallback:** a 1×1 `(128,128,255)` texture is created in `init` and bound at sampler
  slot 1 whenever a sprite has no normal map. The two-sampler pipeline *must* always have both bound
  or D3D12 errors.
- `assets/shaders/sprite.hlsl` (the unlit M1 shader) is still in the repo and compiled but no
  longer loaded. `lit_sprite.hlsl` replaced it.

## 3. Textures (`resources/`)

- `Texture{handle, width, height}`. `load_texture` decodes a PNG with **stb_image** to RGBA8 and
  uploads it through a transfer buffer + copy pass into an `R8G8B8A8_UNORM` sampler texture.
  `stb_image_impl.cpp` is the single TU defining `STB_IMAGE_IMPLEMENTATION`, compiled with `/w`.
- `TextureRegistry` owns all textures and hands out **`TextureHandle`** (an index;
  `kInvalidTexture` = none). It deduplicates by path. Components store handles, never GPU pointers.
  This is the seam for hot-reload/atlasing later.
- Color is handled in gamma space for now (albedo and normal maps both `_UNORM`; normal maps are
  data and must never be sRGB). Full linear-space color management is deferred.

## 4. Lighting (2.5D, normal-mapped)

`assets/shaders/lit_sprite.hlsl`. Lighting happens in **world space (meters)**; the sprite plane
is z = 0 and the view looks straight down (`V = (0,0,1)`).

Per fragment:
1. Sample albedo. If `emissive`, return it unchanged (**unlit**: the void backdrop, debug viz,
   beams).
2. Unpack the normal `n = tex*2−1` and **rotate `n.xy` by the sprite's `(cos, sin)`**, the *same*
   values the CPU baked into the positions (not the inverse). `N = normalize(nr, n.z)`. This keeps
   lighting world-consistent as a ship turns: a fixed light keeps lighting the same world-facing
   side.
3. `lit = albedo · ambient.rgb · ambient.a`.
4. For each active light (≤ 16):
   - **Point:** `L = normalize(light.xy − P.xy, height)`; attenuation `saturate(1 − dist/range)`.
   - **Directional:** `L = normalize(dir.xy, height)`; no attenuation.
   - Diffuse `albedo · radiance · N·L`, plus Blinn-Phong specular
     `radiance · pow(N·H, shininess) · specStrength · N·L`.
5. Output `(lit, albedo.a)`. Alpha is never lit.

**Height is the headline knob:** low lights graze surfaces dramatically; high lights wash flat.

### Lights → GPU

- Lights are ECS entities: `Transform` (position) + `Light{type, color, intensity, height, range,
  direction}`. Sources: the level's `"lights"` array + an ImGui-draggable debug light.
- `collect_lights(registry, ambient, params)` packs them into `LightsUBO` (`render/LightsUBO.hpp`):
  `ambient`, `params` (x = shininess, y = specular strength), `counts.x`, and 16 × `GpuLight`
  (3 `float4` lanes each). **The C++ struct must byte-match the HLSL cbuffer**; a `static_assert`
  guards the size.
- Collected **at render time**, never inside the fixed-step loop. Lighting is presentation.
- Cost: the shader loops only `count` lights. The biggest cost is full-screen floor × lights.

### Normal maps

- Each sprite may have `<name>_normal.png` (same size + frame layout as the albedo), referenced
  from its definition JSON (`"normal"`). Missing → flat fallback.
- Generated procedurally by `tools/gen_normals.ps1` from the albedo (height = alpha × (0.5 + 0.5 ×
  luminance), central-difference gradient). **Green-channel convention:** DirectX-style, image rows
  go down = world −Y. If lighting ever looks inverted, flip the sign in that one script.

## 5. Render-side systems

- `render_collect(registry, renderer, alpha)` iterates `PrevTransform + Transform + SpriteRender`:
  - lerps position and rotation by `alpha` (**plain lerp** on rotation, not shortest-arc),
  - picks the tilt frame for 3-frame sheets from `ShipControl.turn` (> 0.2 → bank left frame 0,
    < −0.2 → frame 2, else neutral 1),
  - scales by `Altitude.visualScale` if present (fall-away cue),
  - queues the sprite with its normal handle.
- The **void backdrop** is one unlit, camera-locked quad (`background.png`) sized 1.2× the view,
  drawn first. Floor rects (lit) draw over it; pits are floor gaps that reveal it. Parallax later =
  offset the backdrop by `camera × factor`.
- **Debug ray** and **beams** are thin unlit quads using `debug_white.png` (no per-draw tint yet, so
  `BeamParams.color` is stored but not applied).

## 6. Camera (`render/Camera.hpp`, `render/CameraFollow.hpp`)

- `Camera2D{position (m), zoom, pixelsPerMeter = 48}`. `view_proj(w, h)` = ortho with half-extents
  `(viewport/2) / (pixelsPerMeter · zoom)`, +Y up, then translate by −position.
  `GLM_FORCE_DEPTH_ZERO_TO_ONE` matches SDL_GPU's 0..1 clip depth.
- **Follow is elastic and translation-only** (the camera never rotates). `CameraFollow` is a small
  polymorphic interface; `App` holds one in a `unique_ptr`, switchable in the Camera panel:
  - `SmoothDampFollow` (critically damped spring, `smoothTime`, no overshoot, default),
  - `ExponentialFollow` (frame-rate-corrected exponential lag, `responsiveness`).
- It targets the ship's **interpolated** render position with the **real frame dt** (render time).

## 7. ImGui

- Backends: `imgui_impl_sdl3` (events/input) + `imgui_impl_sdlgpu3` (drawing; needs ImGui ≥ 1.91.6).
  Built as a static lib by `cmake/imgui.cmake`.
- Every SDL event goes to `ImGui_ImplSDL3_ProcessEvent` first. Game keyboard sampling is skipped
  while `ImGui::GetIO().WantCaptureKeyboard` is true.
- Two-stage submit: `PrepareDrawData` before the render pass, `RenderDrawData` inside it, after the
  world, so UI draws on top.
- `imgui.ini` (window layout) is generated at runtime and git-ignored.

## 8. Gotchas

- Uploads before `BeginGPURenderPass`. Always.
- End a copy pass with `SDL_EndGPUCopyPass(copyPass)`, not the command buffer.
- Normal rotation sign: rotate by the same `(c, s)` as the positions.
- Keep `LightsUBO` and the cbuffer in sync (and the `static_assert`).
- Both fragment samplers bound for every draw.

## 9. Planned

SDL_shadercross build step (all platforms) · parallax / multi-floor layers (M6) · per-draw tint ·
atlasing if draw-run counts grow · shadows/deferred are out of scope for now.
