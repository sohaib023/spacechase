# M3 — Meters, Collision, Walls & Pseudo-3D Falling

**Status:** ✅ done · **Goal:** make the world physical and populated: continuous **meter** units, a
custom top-down collision layer (spatial-hash broadphase + narrow phase + raycast), static and
destructible walls, a floor with voids, and falling as pseudo-3D vertical physics. Also land the
texture registry + multi-texture batch (walls need different sprites than the ship).

## Decisions locked

| Decision | Choice |
|---|---|
| Units | World unit = **1 meter**. Camera gains `pixelsPerMeter`; ship stats retuned to physical values. Applies to the ship too. |
| Geometry | **Continuous meter space, NOT a tile grid** (user decision). Walls = AABB collider entities. Floor = rectangular regions; void = the gaps. |
| Broadphase | **Spatial hash** (uniform grid over continuous space), purely an accelerator. |
| Rendering | `TextureRegistry` (handles) + **multi-texture batch** (one draw per texture run). |
| Falling | **Pseudo-3D:** ships hover at height H; over void gravity lowers altitude; back over floor first → recover; altitude ≤ 0 over void → death. **`controlWhileFalling` is a flag** (on = steer to recover, off = momentum only). Single layer: no dropping to a lower floor yet. |
| Scope | One milestone in sub-steps: units → rendering → collision + walls → raycast → falling. Includes `Health` + destructible walls (debug-damaged; weapons come in M5). |
| New deps | None. |

## Design

- **`src/physics/`** (glm + ecs only, no SDL): `Shapes` (Circle, AABB), `Overlap` (manifolds,
  normal pushes A out of B), `SpatialHash` (id-sorted queries), `Raycast`, `ForceApi` (stub for M5).
- **Components:** `Hitbox`, `Health`, `CollisionLayer{layer, mask}`, `StaticTag`, `Wall{destructible}`,
  `Altitude{z, vz}`.
- **Systems:** `collision_step` (rebuild hash → swept broadphase → narrow phase → push-out + slide);
  `floor_altitude_step` (+ `apply_fall_control`).
- **Data:** `WallDef` + `load_walls_from_dir` (shared `load_defs` template); `LevelDef` + `LevelLoader`
  → `assets/levels/test_m3.json`; `gameplay/Level` (`has_floor`), `FallConfig`, `WallFactory`.

### Pipeline placement
```
snapshot_prev → input_apply → apply_fall_control → movement_step
→ collision_step (NEW) → floor_altitude_step (NEW)
```
Collision resolves **after** integration; render lerps prev → corrected.

### Collision response
Closest-point manifold; push out along the normal; cancel inward velocity (stop head-on, slide when
glancing, no bounce); deepest-first, ≤ 3 iterations, small slop; stable order.

## Sub-steps

1. Units → meters. 2. TextureRegistry + multi-texture batch. 3. Shapes/Overlap/SpatialHash + tests.
4. CollisionSystem + walls. 5. Raycast + debug draw + tests. 6. Floor + falling. 7. Health + debug
"destroy crate" + LevelLoader/JSON. 8. Docs + commits.

## Risks called out

Determinism (sort candidates by id) · tunneling (thick walls + swept broadphase; no CCD) · jitter
(skin slop) · interpolation vs corrected transform (snapshot before movement) · draw order without
depth (floor → walls → ships) · resist multi-floor creep (that's M6).

## Outcome

Delivered, with changes along the way:
- **Floor model:** the plan had explicit void rects; post-M4 it became **floor rects only, void = the
  gaps**, drawn as one unlit backdrop.
- **Ship hitbox:** planned as a circle; before M5 it became the ship's real **convex polygon**, with
  SAT (`convex_vs_convex`, `circle_vs_convex`) added to the narrow phase.
- Death currently respawns at the level spawn (placeholder).
- The debug "damage nearest crate" button stood in for weapons until M5.
