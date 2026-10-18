# Space Chase — Game Design & Decisions

The non-technical picture: what the game is, how it should feel, and the design decisions made so
far, with the reasons behind them. For the original planning input see
[`groundwork-brief.md`](groundwork-brief.md). For how it's built see [`../technical/`](../technical/).

---

## 1. The game in one paragraph

**Space Chase** is a 2D, top-down space-ship shooter. You pilot a ship with real momentum: thrust,
turn and drift. You fight through arenas made of walls, destructible crates, bottomless voids, wind
currents and (later) portals and stacked floors. Lighting is dynamic: sprites are flat, but lights
with a *height* above the play field shade them like 3D objects ("2.5D lighting"). The long-term
goal is a **huge, growing variety of ships and abilities** that combine freely, plus a **level
editor** shared by developers and players, and multiplayer.

## 2. Pillars

1. **Weighty, slippery flight.** Ships have momentum and drag. Mastery is about managing drift, not
   point-and-click movement.
2. **Combinatorial weapons.** Abilities and modifiers mix freely (a multifire, quickfire,
   autofire blaster). Variety comes from combining, not from hand-made one-offs.
3. **A dangerous arena.** Walls block, crates break, voids kill (with a chance to recover), and
   force fields push you around.
4. **Readable 2.5D atmosphere.** Moody, normal-mapped lighting that stays consistent as ships turn.
5. **Built for sharing (later).** Levels authored in an editor that players can use too; local and
   then networked multiplayer.

## 3. Platforms & scope

- Desktop first (Windows now; Linux/macOS planned), with **cross-play** in mind.
- Architected so a **mobile port** (Android/iOS) stays cheap: input is abstract *actions*, never
  raw keys.
- **Single-player / same-machine first**, networking later (planned toward an authoritative server).
- Deliberately lean: no general-purpose engine features the game doesn't use.

---

## 4. Core mechanics (as built through M5)

### Ships & flight
- Thrust along the nose, turn left/right, drag bleeds speed, top speed is capped. Handling stats
  (acceleration, max speed, turn rate, drag) come from each ship's definition file and can be tuned
  live in the debug panel.
- The ship sprite **banks** (tilts) left/right while turning.
- The ship's collision shape is its real **polygon** outline, not a circle.

### World & space
- The world is measured in **continuous meters**, not a tile grid. Walls, floors and pits are
  rectangles of any size placed anywhere.
- **Walls** stop the ship and let it slide along them (no bouncing). **Crates** are walls with
  health; shoot them and they break.
- The **floor** is a set of lit platforms. Gaps between them are the **void**, a single unlit
  starfield backdrop "off into infinity" (parallax later).

### Falling (pseudo-3D)
- Ships hover just above the floor. Fly over the void and the ship starts to **fall**: gravity pulls
  its altitude down and the sprite shrinks as it drops away.
- Get back over floor before you hit bottom and you **recover**. Otherwise you **die** (currently:
  respawn at the start).
- Whether you can still thrust while falling is a **tunable flag**: *on* = steer to recover,
  *off* = momentum carries you. Kept switchable on purpose.

### Camera
- The camera **follows the ship elastically** (eases in, lags, settles) and **only translates**. It
  never rotates with the ship. Two follow models exist (smooth spring / exponential lag),
  swappable for feel-testing.

### Lighting
- Point and directional lights, each with a **height** above the floor. Height is the main mood knob:
  low lights rake across surfaces dramatically, high lights wash flat.
- Surfaces respond to light via normal maps, and that response **rotates with the sprite**, so a
  fixed light keeps lighting the same world-facing side of a turning ship.
- The void is **never lit**. It's a backdrop, not a surface.

### Weapons (abilities)
- **Blaster** (Space): fires straight projectiles. By default one shot per press.
- **Machine-gun beam** (Shift): a continuous hitscan beam that stops at the first wall and deals
  damage per second to what it touches.
- **Modifiers** reshape a weapon without making a new one:
  - *Multifire*: several shots in an angular spread.
  - *Quickfire*: shorter cooldown (optionally faster shots).
  - *Autofire*: hold to keep firing.
- Shots never hit their own shooter. Bullets and beams destroy crates.
- **Ricochet:** surfaces are either **reflective** or **absorbent**. Bouncy bullets and beams
  bounce off reflective surfaces and keep going, dealing full damage on *every* hit with no loss
  of speed or damage. Absorbent surfaces stop them. Whether something bounces, and how many times
  (a limit or unlimited), is a property of each ability. Which surfaces reflect is level data.
  Today the base blaster bounces without limit, the beam bounces twice, plain walls reflect and
  crates absorb.
- **Range:** every bullet, beam and ability has a maximum travel distance, counted along its whole
  path including bounces. The blaster reaches 40 m and the beam 30 m.

### Force fields
- One mechanic with many faces: **wind zones**, the ship's **deflector**, and (later) chi-push are
  all *force fields*: a push that can be overcome with enough thrust, not a hard wall.
- **Wind zones** are placed in levels and push anything movable.
- The **deflector** is a *ship ability* (a toggle) that repels nearby movable things, such as
  incoming bullets. It isn't a level object; that would duplicate the same mechanic.
- Every object is explicitly **Rigid** (never pushed: walls, crates) or **Deflectable** (pushed:
  ships, bullets).

---

## 5. Design decisions log

Decisions taken while building, and why. (Technical decisions live in the technical docs; these are
the ones with a design reason.)

| Decision | Choice | Why |
|---|---|---|
| World geometry | Continuous meters, **not** a tile grid | Free-form level shapes; physics in real units; same rules for ships, walls and pits. |
| Ship stats | Physical units (m/s, m/s²) | Tuning stays intuitive and scale-independent. |
| Falling | Recover-or-die, with a `controlWhileFalling` flag | Gives a skill moment (save yourself) while keeping the harsher variant one switch away. |
| Void look | One unlit backdrop, pits are floor gaps | The void is "infinity", not a surface. Lighting it looked wrong. |
| Camera | Elastic follow, translate only | Rotating cameras disorient in top-down shooters; elasticity sells speed. |
| Ship collision | Real polygon hitbox | A circle felt wrong on a triangular ship (corners clipping or floating off walls). |
| Abilities | **Defined by developers in code, not player-editable** | Unlike levels, abilities are a balance surface we control. The composable pipeline is kept because it's good engineering, not for modding. |
| Deflector | A ship ability reusing the force-field system | Avoids a second implementation of the same push mechanic. |
| Force response | Every object explicitly Rigid or Deflectable | Makes "what can be pushed" a visible, deliberate property of each object. |
| Default fire mode | Semi-auto (one shot per press) | Autofire is a modifier you earn/equip, not the baseline. |
| Beam damage | Per second while touching | A beam is continuous; per-hit damage would depend on frame rate. |
| Machine gun | A beam, not thousands of bullets | Cheaper and more readable. The cone version (partly blocked by walls) is planned as several thin beams. |
| Ricochet | Composable per ability: bounce on/off, bounce limit or unlimited | Lets bouncing be mixed into any weapon, the same way as modifiers. |
| Bounce penalty | None: full speed and damage after every bounce | Keeps ricochet shots strong and readable. Abilities that lose power (chi) are a separate trait. |
| Surface type | Reflective or absorbent, set per surface in data | Level design decides where shots bounce, without touching weapon code. |
| Range | A max travel distance on every bullet/beam/ability, separate from bouncing | Bounded shots without coupling range to the ricochet logic. |

---

## 6. Planned features (not built yet)

From the roadmap ([`../plans/README.md`](../plans/README.md)):

- **Environment depth (M6):** portals (fly in, come out at the linked one), machinery that explodes
  when destroyed, **multiple floor layers** with parallax (fall *down* to a lower floor instead of
  dying), optional pseudo-3D walls between floors.
- **Level editor (M7):** place and configure everything visually. It enumerates the same content
  definitions the game loads, so new content appears in it automatically. Devs first, players later.
- **Audio, HUD and menus (M8).**
- **Networking (M9):** online multiplayer on top of the same simulation.
- **More abilities:** missiles/rockets, homing/heatseeking (needs enemies to target), deployables
  (mines, laid lasers), shields, a cone machine-gun, movement abilities (boost, blink).
- **Chi:** a nearly transparent ability, visible only because it slightly **warps the space** it
  travels through. It **passes through walls** and other obstacles, but **loses speed and damage**
  each time it does. Unlike ricochet shots (which never weaken), it's the first ability that
  degrades. It will be built as its own projectile trait on top of the same hit system.

## 7. Open design questions

Still to be decided (carry these into the relevant milestone's planning):

1. **Multiplayer shape:** co-op or competitive PvP? How many players? This decides how strict the
   simulation's determinism must be.
2. **Chi / space-warp:** besides passing through walls with speed/damage loss, does its warp
   also push things?
3. **Pseudo-3D walls between floors:** in scope for the first version?
4. **Editor:** in-game panel or separate program? How polished for players?
5. **Scale targets:** how many bullets and entities on screen at once? This drives performance work.
6. **Death & scoring:** what happens on death beyond respawning? Lives, scores, rounds?
7. **Enemies / AI:** none yet. Homing weapons and PvE both depend on this.
