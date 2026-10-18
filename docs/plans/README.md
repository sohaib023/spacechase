# Plans & Roadmap

Every milestone gets a plan here **before** it's built. The plan files are the historical record of
*what we decided and why*. Each one ends with an **Outcome** section noting where reality deviated.
For how the code works *now*, see [`../technical/`](../technical/), which is kept current.

## Roadmap

Build order from the groundwork brief (§9), with status:

| # | Milestone | Status | Plan |
|---|---|---|---|
| M0 | Bootstrap: CMake + deps, SDL3 window + GPU device, ImGui, logging | ✅ done | [m0-bootstrap.md](m0-bootstrap.md) |
| M1 | Render + move one ship: sprite batch, camera, fixed timestep, input, momentum/drag, tilt sheet | ✅ done | [m1-render-and-move.md](m1-render-and-move.md) |
| M2 | ECS + data spine: EnTT, system pipeline, ship stats from JSON, doctest | ✅ done | [m2-ecs-and-data.md](m2-ecs-and-data.md) |
| M3 | World & collision: meters, textures, physics layer, walls, raycast, falling, levels | ✅ done | [m3-collision-and-falling.md](m3-collision-and-falling.md) |
| M4 | Normal-mapped 2.5D lighting (+ void/camera fixes, polygon hitboxes) | ✅ done | [m4-lighting.md](m4-lighting.md) |
| M5 | Abilities, projectiles, modifiers, beam, force fields + deflector | ✅ done | [m5-abilities-and-forces.md](m5-abilities-and-forces.md) |
| M6 | Environment depth: portals, machinery/explosions, multi-floor + parallax (+ optional pseudo-3D walls) | ⏳ next | *to plan* |
| M7 | Level format + editor (ImGui), sharing the registries | planned | *to plan* |
| M8 | Audio, HUD/UI | planned | *to plan* |
| M9 | Networking (ENet) on the multiplayer-shaped sim | planned | *to plan* |
| — | SDL_shadercross build step (cross-platform shaders) | deferred, own milestone, needed before Linux/macOS | *to plan* |

## Carried-over backlog (decided-but-deferred)

- **SDL_shadercross**: replace `tools/compile_shaders.ps1` with a build step emitting
  DXIL/SPIR-V/MSL.
- **Hot-reload** of definitions and textures.
- **Static-library layer split** (core/render/ecs/data) once the editor/net give a second consumer.
- Abilities: deployables, chi/warp, shield, homing (needs enemies), cone machine gun.
- Hash-accelerated + rotated-shape raycasts. Persistent spatial hash if counts grow.
- Real death/scoring (falling currently respawns). Per-draw sprite tint.
- Open design questions: [`../game-design/game-design.md`](../game-design/game-design.md#7-open-design-questions).

## How we plan & execute

1. **Plan** the milestone in plan mode: context, locked decisions (with alternatives considered),
   files/components/systems, pipeline placement, ordered sub-steps, verification, risks, out of
   scope. Questions that change the design are asked up front.
2. After plan revisions, **don't jump straight to approval**. The user reads the reasoning first,
   then explicitly asks for the plan.
3. Save the approved plan here as `mN-<topic>.md`.
4. **Build in sub-steps** (M5.1, M5.2, …). Each one compiles warning-free, adds tests for its pure
   logic, passes all tests, and is **committed on its own**.
5. At the end: update the relevant `docs/technical/*` files and fill in the plan's **Outcome**.
