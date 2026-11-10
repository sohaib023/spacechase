# Space Chase — agent notes

Top-down 2D space shooter in C++20 on SDL3 (GPU API, D3D12/DXIL), EnTT, glm, Dear ImGui,
nlohmann/json, doctest. Windows + clang-cl. **All documentation is in [`docs/`](docs/README.md)**.
Start with **`docs/status.md`** (current state + next steps), then `docs/README.md` and
`docs/technical/architecture.md`. Update `docs/status.md` at the end of every session.

## Build & test (PowerShell)

Tools are NOT on a plain PowerShell PATH. Load the VS dev environment in the same command:

```powershell
$env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
Set-Location "<repo root>"
cmake --preset windows-clang-cl
cmake --build --preset windows-clang-cl-debug
.\build\windows-clang-cl\bin\Debug\SpaceChase_tests.exe
```

- Kill a running game first or linking fails: `Get-Process SpaceChase -ErrorAction SilentlyContinue | Stop-Process -Force`.
- Warnings are errors. New `.cpp` files must be added to `CMakeLists.txt` (and to `SpaceChase_tests` if tested).
- After editing `assets/shaders/*.hlsl`, run `tools/compile_shaders.ps1` and commit the `.dxil`.

## Working agreements

- **Plan per milestone** (`docs/plans/`). After revising a plan, summarize and **wait for the user to
  explicitly ask** before presenting it for approval (don't rush ExitPlanMode).
- **Build in sub-steps; commit after each one** that compiles and passes tests.
- The user playtests and reports back. Unit-tested is not "verified" until they confirm it.
- **Commit messages: plain ASCII, no double quotes, no em-dashes** (they break the scripted
  PowerShell git command).
- Keep `docs/technical/` in sync with code changes; new plans go to `docs/plans/`; design decisions
  get logged in `docs/game-design/game-design.md`.
- Prefer **explicit ECS components/enums** for design attributes over implicit tag presence.
- Entity create/destroy during the fixed step goes through `Deferred` + `commit_deferred`, never
  mid-iteration.
- Abilities are code-defined (not JSON). Ships/walls/levels are JSON.
