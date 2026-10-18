# M2 — ECS + Data Spine

**Status:** ✅ done · **Goal:** turn the ship into an EnTT entity of POD components advanced by an
explicit ordered system pipeline, load its stats from a JSON definition through a typed registry,
and add a doctest harness. Behavior must stay **identical** to M1.

## Context

M1's ship was a plain struct updated by free functions. M2 lays the ECS and data spine now, with
one simple ship, before M5's abilities make it mandatory. Retrofitting mid-flight is the risk
being avoided.

## Decisions locked

| Decision | Choice |
|---|---|
| ECS library | **EnTT v3.15.0** (header-only, lean). **flecs** considered and declined: more batteries (reflection, relationships) but heavier; EnTT's `entt::meta` can cover editor reflection later if needed. |
| Data format | **JSON** via nlohmann/json v3.12.0. |
| Build target | Keep **one `SpaceChase` executable**; defer the static-library split until there are real separate consumers. |
| Tests | **doctest v2.4.11**, separate `SpaceChase_tests` target, registered with CTest. |
| Registry ownership | `ecs/World { Registry registry; Entity playerShip; }` owned by `App`. **No global**, so the sim stays self-contained and replication-friendly. |

## Design

- **Components** (`src/components/`, one POD per header): `Transform`, `PrevTransform`, `Velocity`,
  `ShipControl` (per-entity intent that input/AI/net write), `ShipStats`, `SpriteRender`,
  `PlayerControlled`.
- **`ecs/Registry.hpp`**: the single EnTT include seam (`sc::Registry`, `sc::Entity`).
- **Systems** (free functions taking `Registry&`): `input_apply`, `snapshot_prev_transforms` +
  `movement_step` (verbatim M1 math), `render_collect`.
- **Data**: `ShipDef` (JSON-free header) + `DefinitionRegistry` (`load_ships_from_dir`, `find_ship`;
  log-and-skip bad files). `assets/definitions/ships/player_ship.json` with the exact M1 literals.
- **`gameplay/ShipFactory::spawn_ship`**: composes the entity, `PrevTransform == Transform`.
  `Ship.{hpp,cpp}` retired.

## Migration order

1. Deps only (verify clean under `-Werror`) → 2. components + `ecs/` → 3. data layer + JSON →
4. systems + factory alongside the old path → 5. switch over → 6. retire `Ship.*` →
7. tests → 8. docs + commits.

## Risks called out

- Rotation interpolation must stay a **plain lerp** (not shortest-arc) to match M1.
- `snapshot_prev_transforms` runs **inside** the step loop before each `movement_step`.
- First frame: `PrevTransform == Transform` from the factory.

## Verification

Flies exactly like M1. Editing `maxSpeed` in the JSON changes handling without recompiling. Tests
green.

## Outcome

Delivered as planned. In M3, `input_apply` moved *inside* the fixed-step loop, and `SpriteRender`'s
raw texture pointer became a `TextureHandle`.
