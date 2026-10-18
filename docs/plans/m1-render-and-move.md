# M1 — Render + Move One Ship

**Status:** ✅ done · **Goal:** a 2D sprite batch on the SDL_GPU API, an orthographic camera, a
fixed-timestep simulation decoupled from rendering, action-based input, and a momentum/drag ship
with a tilt sprite sheet.

> This plan is reconstructed from the end-of-milestone guide; M1 was planned in conversation and
> not saved as a plan file at the time.

## Decisions

| Decision | Choice | Why |
|---|---|---|
| Shaders | HLSL **hand-compiled to DXIL** (`tools/compile_shaders.ps1`), device requests DXIL only (→ D3D12) | Get pixels on screen now. SDL_shadercross deferred to its own milestone. |
| Math / images | **glm 1.0.3**, **stb_image** (pinned commit) | Header-only, minimal. stb chosen over SDL3_image (fewer deps, all we need is PNG decode). |
| Timestep | Fixed **60 Hz** with accumulator, frame time clamped to 0.25 s; render interpolates prev→current by `alpha` | Frame-rate-independent, stable physics. Prerequisite for netcode. |
| Interpolation | Plain lerp on position and rotation | Simple. Kept identical in later milestones. |
| Input | Abstract `InputActions{thrust, turn}`; key mapping only in `App` | Cheap gamepad/touch port later. |
| Ship art | Procedural 3-frame tilt sheet (bank-left/neutral/bank-right), 192×64 | Placeholder art, frame picked from turn input. |

## Scope

- `render/Renderer` (pipeline, sampler, dynamic VBO + transfer buffer; `begin/draw/upload/render`),
  `render/Camera.hpp` (`Camera2D`, ortho, +Y up).
- `resources/Texture` (stb decode → GPU upload via copy pass), `stb_image_impl.cpp` (compiled `/w`).
- `gameplay/Ship` (`ShipConfig`, `ShipState`, `ship_step`, `ship_lerp`). Replaced by ECS in M2.
- `assets/shaders/sprite.hlsl` + compiled DXIL, `assets/sprites/ship_sheet.png`.
- `App::run` rewritten into the fixed-step loop; an ImGui "Ship" tuning panel.

## Verification

Ship flies with WASD/arrows, banks while turning, drifts with momentum; handling tunable live;
smooth at any frame rate.

## Outcome

Delivered. Bugs worth remembering (now documented in [`../technical/rendering.md`](../technical/rendering.md)):
- SDL_GPU matches vertex inputs by **`TEXCOORD<n>` = attribute location**. `POSITION` caused D3D12
  `E_INVALIDARG`.
- `SDL_EndGPUCopyPass` takes the copy pass, not the command buffer.
- All GPU uploads must happen **before** the render pass begins.
