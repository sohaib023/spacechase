# Project Status & Handover

**The "you are here" page.** Read this first when picking the project up (new agent, new PC,
after a break). Update it at the end of every work session or milestone.

*Last updated: 2026-11-11 · Linux port done (see [`plans/linux-port.md`](plans/linux-port.md)) · branch `master`*

---

## 1. Where things stand

- **Milestones M0–M5 and M5b are done** (see [`plans/README.md`](plans/README.md)). The game is
  a flyable, lit arena with walls, breakable crates, voids you can fall into, wind zones, a
  deflector ability, a ricocheting blaster with modifiers, and a bouncing machine-gun beam.
- **Build:** clean with warnings-as-errors under clang-cl (Windows) and clang 19 (Ubuntu 24.04).
  **73/73 unit tests pass** on Linux.
- **Platform:** Windows (D3D12/DXIL) and Ubuntu (Vulkan/SPIR-V). Ubuntu setup is one script,
  `tools/setup_ubuntu.sh`. macOS waits on MSL (SDL_shadercross milestone).

### Verification status

| Feature | Unit tests | Playtested by the user |
|---|---|---|
| Flight, collision, walls, falling, lighting, camera (M1–M4) | ✅ | ✅ |
| Blaster, crate destruction, walls stop bullets, modifiers, beam, wind (M5) | ✅ | ✅ |
| Deflector bending *incoming* bullets | ✅ | ❌ not yet. Use the **"fire test volley at ship"** button in the Ship panel |
| Mouse fire bindings (keyboard-ghosting workaround) | — | ❌ not yet |
| Ricochet, reflective/absorbent surfaces, range, bouncing beam (M5b) | ✅ | ❌ not yet. The game launches cleanly; feel unconfirmed |
| Linux/Vulkan rendering (SPIR-V shaders) | — | ❌ not yet. Launches and initializes on Vulkan (Intel Arc under Wayland); visuals unconfirmed |
| Windows after the Linux port (shader-format selection, `compile_shaders.cmake`, `normal_maps` target) | — | ❌ not yet rebuilt on Windows |

## 2. Immediate next steps

0. **Linux port follow-ups:** play on Ubuntu and confirm rendering/lighting match Windows. Check
   whether the default X11 path gets the real GPU from a normal terminal (inside the agent sandbox
   it fell back to llvmpipe; `SDL_VIDEO_DRIVER=wayland` uses the Intel GPU). Rebuild on Windows
   once to confirm nothing regressed.
1. **Playtest the unverified rows above** and fix whatever comes up. In the Ship panel:
   - *bounce* section: toggle bounce / max bounces / range for blaster and beam,
   - *deflector ability* + *fire test volley at ship*.
   Expected behavior is in [`technical/abilities-and-forces.md`](technical/abilities-and-forces.md).
2. **Plan M6, Environment depth** (portals, machinery/explosions, multi-floor + parallax,
   optional pseudo-3D walls). Follow the workflow in [`plans/README.md`](plans/README.md). Settle
   these with the user before writing the plan:
   - Scope split: all of M6 at once, or portals + machinery first and multi-floor later?
   - Multi-floor: falling to a lower floor replaces death? How many layers? Do layers have
     separate walls and lights?
   - Portals: do projectiles/beams pass through portals too? Does momentum carry through?
   - Machinery: explosion damage radius/falloff (reuse a radial force field for the blast push?).
   - Pseudo-3D walls between floors: in or out for v1 (brief says toggleable)?
3. Open design questions that later milestones depend on:
   [`game-design/game-design.md` §7](game-design/game-design.md#7-open-design-questions).

## 3. Known limitations / rough edges

- Raycasts ignore AABB rotation and treat polygon hitboxes as circles (fine today: only unrotated
  walls are raycast targets). [`technical/physics-and-collision.md` §7](technical/physics-and-collision.md)
- Beam color is stored but not drawn (no per-draw tint in the renderer).
- Ricocheting bullets cut the corner visually for one sub-step.
- Range can overshoot by ≤ one step (~0.4 m). Accepted.
- Death by falling just respawns at the level spawn (no lives/score yet).
- No enemies/AI. Firing is wired to `PlayerControlled` + keyboard/mouse input only.
- The level is hard-coded (`assets/levels/test_m3.json`); no level selection.
- The "damage nearest crate" debug button predates weapons and is now redundant.
- No hot-reload of definitions/textures yet.
- `fire_step` (a system) includes `gameplay/Abilities.hpp`: a known layering bend, fine while
  single-target.

## 4. Things that aren't in the code but matter

- **Remote:** `origin` = https://github.com/sohaib023/spacechase, branch `master` (tracking).
  On a new PC: `git clone https://github.com/sohaib023/spacechase.git`, then follow
  [`getting-started.md`](getting-started.md). Only push when the user asks.
- **Working agreements with the user** (also in the root [`CLAUDE.md`](../CLAUDE.md)):
  - Plan each milestone first. Ask the design questions up front, keep plans brief when asked.
    After revising a plan, summarize and **wait for the user to ask** before presenting it for
    approval.
  - Build in sub-steps; **commit after each** green sub-step; plain-ASCII commit messages.
  - The user playtests and reports back. Don't treat unit-tested features as verified until then.
  - Keep features **composable** (per-ability traits, explicit ECS components/enums such as
    `ForceResponse`, `SurfaceResponse`) and keep separate concepts separate (e.g. range vs bounce).
  - All documentation lives in `docs/`. Keep `technical/` in sync with code, log design decisions
    in `game-design/game-design.md`, and update **this file** at the end of each session.
- **Environment quirks:** see [`getting-started.md`](getting-started.md) §2, §5 and §7 (dev shell
  PATH, `vswhere` noise, kill the running exe before relinking, keyboard ghosting, Linux X11 vs
  Wayland).
- **Committed DXIL** was last built by the Windows SDK dxc. Running `compile_shaders.cmake` on
  Linux rewrites it with a newer dxc; that output hasn't been tested on Windows yet.

## 5. Quick orientation

| I want to… | Read |
|---|---|
| set up and build | [`getting-started.md`](getting-started.md) |
| understand the game | [`game-design/game-design.md`](game-design/game-design.md) |
| understand the code | [`technical/architecture.md`](technical/architecture.md) → module docs |
| see why something was decided | [`plans/`](plans/) (per-milestone decisions + outcomes) |
| add a ship/wall/level/ability | [`technical/data-and-content.md`](technical/data-and-content.md) §4, [`technical/abilities-and-forces.md`](technical/abilities-and-forces.md) §8 |
