# Data & Content

How content is defined as data, loaded, and turned into entities. Code: `src/data/`,
`src/gameplay/*Factory.*`, `assets/definitions/`, `assets/levels/`.

---

## 1. Principle

Ships, walls and levels are **JSON files read by generic code**: change a number, relaunch, no
recompile. The runtime and the future level editor (M7) read the **same registries and the same
level format** (one source of truth). The editor will enumerate the registries to build its palette,
so new content shows up in it automatically.

**Exception: abilities are code-defined** (`gameplay/Abilities.cpp`). They're a developer-controlled
balance surface, not player content. See [`abilities-and-forces.md`](abilities-and-forces.md).
Force fields and lights *are* data, placed in levels.

## 2. Pipeline

```
assets/definitions/<kind>/*.json
   │  DefinitionRegistry::load_<kind>s_from_dir   (nlohmann/json, log-and-skip bad files)
   ▼
typed Def structs keyed by id  (ShipDef, WallDef)
   │  find_<kind>(id)                + resolve texture paths → TextureHandles (App)
   ▼
gameplay/<Kind>Factory::spawn_<kind>(registry, def…)  → entity with components

assets/levels/<level>.json
   │  load_level(path, LevelData&)   (GPU-free → unit-testable)
   ▼
LevelData { Level floor, playerSpawn, walls[], lights[], forceFields[] }
   │  App::init instantiates: walls (via WallDef + WallFactory), player ship, lights, fields
```

- **Def headers are JSON-free** (`ShipDef.hpp`, `WallDef.hpp`, `LevelDef.hpp`); parsing lives in
  `DefinitionRegistry.cpp` / `LevelLoader.cpp` so nlohmann/json stays out of headers.
- `load_defs<Def>(dir, kind, map, parseFn)` is the shared template: every `*.json` in the folder,
  parse errors and missing `id`s are logged and skipped, returns true if ≥ 1 loaded.
- All asset paths in JSON are **relative to `assets/`** (`SC_ASSET_DIR`, which points at the source
  tree in dev builds).
- The level loaded at startup is currently hard-coded: `assets/levels/test_m3.json`.
- No hot-reload yet (planned; the `TextureRegistry`/registries are the seams).

## 3. Schemas

### Ship: `assets/definitions/ships/*.json`
```json
{
  "id": "player_ship",                       // required, unique
  "displayName": "Player Ship",              // optional (defaults to id)
  "sprite": {
    "sheet": "sprites/ship_sheet.png",       // required; horizontal frame strip
    "normal": "sprites/ship_sheet_normal.png", // optional normal map (same layout)
    "frameCount": 3,                         // 3 = bank-left / neutral / bank-right
    "frameSize": [2.0, 2.0]                  // meters
  },
  "hitbox": {                                // optional
    "polygon": [[0.0, 0.9], [0.8, -0.7], [-0.8, -0.7]],  // convex, local meters, ≤ 8 verts
    "radius": 0.5                            // used if no polygon
  },
  "stats": { "acceleration": 40.0, "maxSpeed": 20.0, "turnSpeed": 3.2, "drag": 1.2 }
}
```
`ShipFactory::spawn_ship` emplaces: Transform/PrevTransform, Velocity, ShipControl, ShipStats,
SpriteRender, Hitbox (polygon or circle), CollisionLayer{Ship, Wall}, ForceResponse(Deflectable),
Deflector (inactive), BeamRequest (inactive), Altitude, and PlayerControlled if requested.
`App` then adds the `AbilityLoadout` in code.

### Wall: `assets/definitions/walls/*.json`
```json
{
  "id": "crate",
  "sprite": "sprites/crate.png",
  "normal": "sprites/crate_normal.png",   // optional
  "destructible": true,                   // default false
  "health": 100.0,                        // used if destructible
  "halfExtents": [1.5, 1.5],              // default size (meters); instances may override
  "surface": "absorbent"                  // absorbent (default) | reflective: bullets/beams bounce
}
```
Current defs: `wall` (indestructible, reflective, sized per instance) and `crate` (100 HP,
absorbent).
`WallFactory::spawn_wall` emplaces: Transform/PrevTransform, SpriteRender (size = 2·halfExtents),
Hitbox(AABB), CollisionLayer{Wall, 0}, ForceResponse(Rigid), SurfaceResponse(from `surface`),
StaticTag, Wall{destructible}, and Health if destructible.

### Level: `assets/levels/*.json`
```json
{
  "playerSpawn": [0.0, 0.0],
  "floor": [ { "center": [0.0, -9.0], "halfExtents": [16.0, 3.0] }, ... ],   // gaps = void
  "walls": [
    { "def": "wall",  "center": [8.0, 0.0], "halfExtents": [1.0, 4.0] },    // size override
    { "def": "crate", "center": [-5.0, 3.0] }                                // def's default size
  ],
  "lights": [
    { "type": "directional", "direction": [0.3, -1.0], "height": 4.0,
      "color": [0.4, 0.5, 0.7], "intensity": 0.5 },
    { "type": "point", "position": [-6.0, 4.0], "height": 2.0,
      "color": [1.0, 0.7, 0.4], "intensity": 2.5, "range": 12.0 }
  ],
  "forceFields": [
    { "shape": "directional", "position": [0.0, -2.0], "dir": [1.0, 0.0],
      "strength": 14.0, "range": 7.0, "falloff": "linear" }   // shape: radial|directional
  ]                                                           // falloff: none|linear|inverseSquare
}
```
Defaults where omitted: light type `point`, intensity 1, height 2, range 10; field shape `radial`,
strength 0, range 10, falloff `linear`. All arrays are optional.

This is the **M3–M5 subset** of the eventual editor level format. M7 will formalize it (versioning,
layers, portals, spawn points, entity instances in general).

## 4. Adding content: recipes

- **New ship type:** add `definitions/ships/<id>.json` + sprite (+ optional normal: add it to
  `tools/gen_normals.cpp` and build the `normal_maps` target). Spawn it with `find_ship("<id>")` +
  `spawn_ship`. No code change to the loader.
- **New wall/obstacle type:** add `definitions/walls/<id>.json` + sprite; reference it from a level's
  `walls` by `def`.
- **New level:** copy `test_m3.json`, edit, and point `App::init` at it (until level selection exists).
- **New definition *kind*** (e.g. pickups): add `XDef.hpp` (POD, JSON-free), a `parse_x` +
  `load_xs_from_dir`/`find_x` in `DefinitionRegistry` (reusing `load_defs`), an `XFactory`, and a
  loader test in `tests/test_definitions.cpp`.
- **New level element kind:** add an `XInstance` to `LevelDef.hpp`, a loop in `LevelLoader.cpp`
  (mirror `lights`/`forceFields`), spawn in `App::init`, test in `tests/test_level.cpp`.

## 5. Tests

`test_definitions` (ship/wall defs parse, lookups, missing dir) and `test_level` (floor, walls,
lights, has_floor) load the **real** files under `assets/`, so editing content can break tests on
purpose. Update the expectations alongside deliberate retunes.
