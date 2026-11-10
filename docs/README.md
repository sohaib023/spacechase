# Space Chase — Documentation

All project documentation lives here. Nothing project-related should be documented anywhere else.

## Reading order

0. **[status.md](status.md)**: current state, what's verified, immediate next steps, known
   rough edges. Start here when picking the project up.
1. **[getting-started.md](getting-started.md)**: install the toolchain, set up VS Code, build, run,
   test, debug, troubleshoot.
2. **[game-design/game-design.md](game-design/game-design.md)**: what the game is, how it plays,
   design decisions and open questions (non-technical).
3. **[technical/architecture.md](technical/architecture.md)**: code layout, layering, ECS model, the
   frame/fixed-step pipeline, conventions. Then the module docs as needed.
4. **[plans/README.md](plans/README.md)**: roadmap, status, and the per-milestone plans.

## Map

```
docs/
  README.md                      this index
  status.md                      current state + next steps (update every session)
  getting-started.md             setup → build → run → debug
  game-design/
    game-design.md               vision, mechanics, decisions log, open questions
    groundwork-brief.md          the original planning brief (source document, kept verbatim)
  technical/
    architecture.md              overview: stack, layers, ECS, pipeline, conventions, build/tests
    rendering.md                 GPU, sprite batch, textures, lighting, shaders, camera, ImGui
    physics-and-collision.md     units, shapes/SAT, spatial hash, collision, raycast, falling
    data-and-content.md          JSON definitions, level format, adding content
    abilities-and-forces.md      firing pipeline, modifiers, projectiles, beam, force fields
  plans/
    README.md                    roadmap + planning/execution workflow
    mN-<topic>.md                one plan per milestone (historical record + outcome)
```

## Keeping docs current

- `status.md` is updated at the end of every work session or milestone.
- `technical/` describes the code **as it is now**. Update it in the same change as the code.
- `plans/` is history. Write a plan before a milestone and add its **Outcome** when done; don't
  rewrite old plans to match later changes.
- `game-design/game-design.md` gets a row in the decisions log whenever a design choice is made.
