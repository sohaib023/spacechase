# Getting Started

How to take a fresh Windows machine to building, running, testing and debugging Space Chase.
Read this first on a new PC (or as a new agent) before touching the code.

> **Platform status:** Windows is the only working target today. The renderer requests **DXIL**
> shaders only (D3D12 backend), so Linux/macOS presets exist as stubs but will not run until the
> SDL_shadercross milestone emits SPIR-V/MSL. See [`technical/rendering.md`](technical/rendering.md).

---

## 1. What you need

| # | Component | Why | Provided by |
|---|---|---|---|
| 1 | MSVC build tools + Windows SDK | STL headers, system libs and the linker that `clang-cl` relies on; `dxc` (shader compiler) | VS 2022 Build Tools, "Desktop development with C++" |
| 2 | `clang-cl` (Clang 18+) | our primary compiler | VS Build Tools, "C++ Clang tools for Windows" |
| 3 | CMake ≥ 3.24 + Ninja | build system + generator | VS Build Tools, "C++ CMake tools for Windows" (bundles both) |
| 4 | Git | VCS + FetchContent downloads | [git-scm.com](https://git-scm.com) / `winget install Git.Git` |
| 5 | VS Code + 3 extensions | edit / build / debug | see §3 |

**Key fact:** `clang-cl` is *not* a standalone toolchain on Windows. It's the Clang front-end
producing MSVC-compatible output, and it **needs the MSVC STL headers and the Windows SDK**. That's
why the MSVC build tools are required even though we compile with Clang.

Why clang-cl and not plain MSVC? Clang gives the same diagnostics we'll get on Linux/macOS later,
while clang-cl still emits MSVC-ABI binaries + **PDB** debug info, so VS Code's MSVC debugger
(`cppvsdbg`) works unchanged. Plain MSVC stays available as a fallback preset.

Versions this repo is known to build with: VS Build Tools 2022 17.14, clang-cl 19.1.5,
MSVC 14.44, CMake 3.31.6, Ninja 1.12.1, Windows SDK 10.0.26100.

---

## 2. Install the toolchain (one command)

Run in a normal PowerShell (Windows 11 ships `winget`):

```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --add Microsoft.VisualStudio.Component.VC.Llvm.Clang --add Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset --includeRecommended"
```

What each `--add` does:
- `Workload.VCTools` + `--includeRecommended` → the MSVC compiler **and the Windows SDK** (#1).
- `Component.VC.CMake.Project` → CMake **and Ninja** (#3).
- `Component.VC.Llvm.Clang` + `Component.VC.Llvm.ClangToolset` → `clang-cl` (#2).

If you already have **Visual Studio 2022** (not just Build Tools): open *Visual Studio Installer →
Modify* and tick "Desktop development with C++", "C++ CMake tools for Windows" and "C++ Clang tools
for Windows". **Make sure the install you build with actually contains the Clang component.** On the
original dev machine, a VS Community 17.11 install exists *without* Clang. Use the Build Tools install
at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` instead.

### The "tools not on PATH" gotcha (read this!)

`clang-cl`, `cmake`, `ninja` and the SDK headers/libs are **not on a plain PowerShell's PATH**. They
only appear once the MSVC developer environment (`INCLUDE`/`LIB`/`PATH`) is loaded. If
`clang-cl --version` fails in a normal terminal, **the install is fine**. You just need the dev
environment:

- **Start menu:** open *"Developer PowerShell for VS 2022"*. Or
- **Any PowerShell:** load it in place:
  ```powershell
  $env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"
  & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
  ```
  The `$env:PATH` prefix puts `vswhere.exe` on PATH. Without it the launcher prints
  `'vswhere.exe' is not recognized ...`. That message is **harmless** because the environment still loads, but it's
  noisy and makes the command report a non-zero exit.

**Verify** (inside the dev environment):
```powershell
clang-cl --version   # expect a recent LLVM (18+)
cmake --version      # >= 3.24
ninja --version
git --version
```

---

## 3. VS Code setup

```powershell
code --install-extension ms-vscode.cmake-tools
code --install-extension llvm-vs-code-extensions.vscode-clangd
code --install-extension ms-vscode.cpptools
```

(`.vscode/extensions.json` also recommends these, so VS Code will prompt for them on first open.)

- **clangd** is the IntelliSense provider. On first activation it offers to download the clangd
  server. Accept it.
- **Microsoft C/C++ (`cpptools`)** is installed **only for its debugger** (`cppvsdbg`). Its
  IntelliSense is disabled (`"C_Cpp.intelliSenseEngine": "disabled"` in `.vscode/settings.json`) so
  the two engines don't fight.
- **CMake Tools** reads `CMakePresets.json` (`"cmake.useCMakePresets": "always"`) and **loads the VS
  developer environment automatically**, so the PATH gotcha above never appears inside VS Code.
  Pick the `windows-clang-cl` configure preset in the status bar.

What the committed `.vscode/` files do:

| File | Purpose |
|---|---|
| `settings.json` | presets always; copies the active preset's `compile_commands.json` to `build/` so clangd finds it; clangd args (`--background-index --clang-tidy`); format-on-save with clangd (uses `.clang-format`) |
| `launch.json` | two `cppvsdbg` launch configs: **Debug SpaceChase (clang-cl)** (builds first) and **(MSVC)** |
| `extensions.json` | the three recommended extensions |

**Debug:** set a breakpoint (e.g. in `App::run`), press **F5** with "Debug SpaceChase (clang-cl)".

---

## 4. Build, run, test (terminal)

From the repo root, inside the dev environment:

```powershell
cmake --preset windows-clang-cl                 # configure (first run downloads all deps)
cmake --build --preset windows-clang-cl-debug   # build Debug  (…-release for Release)
.\build\windows-clang-cl\bin\Debug\SpaceChase.exe        # run
.\build\windows-clang-cl\bin\Debug\SpaceChase_tests.exe  # unit tests (or: ctest --preset windows-clang-cl)
```

- **Presets** (`CMakePresets.json`, generator *Ninja Multi-Config*): `windows-clang-cl` (default),
  `windows-msvc` (fallback), `linux-clang` / `macos-clang` (stubs). Build presets add `-debug` /
  `-release`. Test presets: `windows-clang-cl`, `windows-msvc`.
- **Output** (exe + `SDL3.dll` + PDBs) lands in `build/<preset>/bin/<Config>/`. `SDL3.dll` is copied
  next to the exe by a post-build step.
- **Assets** load straight from the source tree via the compile-time `SC_ASSET_DIR` define, so you
  can edit a JSON file or sprite and just relaunch. No rebuild, no copy step.
- **Dependencies** are fetched by CMake FetchContent on first configure (needs internet): SDL3
  `release-3.4.10`, Dear ImGui `v1.92.8`, glm `1.0.3`, stb (pinned commit), EnTT `v3.15.0`,
  nlohmann/json `v3.12.0`, doctest `v2.4.11`. Nothing to install by hand.
- **Warnings are errors** on our targets (`/W4 /permissive- /WX`, see `cmake/warnings.cmake`).
  Escape hatch if a dependency bump trips warnings: configure with `-DSPACECHASE_WERROR=OFF`.

### Controls (current build)

| Key | Action |
|---|---|
| W / ↑, S / ↓ | thrust forward / reverse |
| A / ←, D / → | turn left / right |
| Space | fire1: blaster (one shot per press; hold with *autofire*) |
| Shift | fire2: machine-gun beam |

ImGui panels (**Ship**, **Lighting**, **Camera**) expose live tuning: ship stats, deflector ability,
weapon modifiers, falling control, lights, camera follow model.

---

## 5. Hand-run tools (`tools/`)

These are **not** part of the CMake build. Run them by hand when their inputs change, and commit
the outputs.

| Script | When | Output |
|---|---|---|
| `tools/compile_shaders.ps1` | after editing any `assets/shaders/*.hlsl` | `assets/shaders/compiled/*.dxil` (uses the Windows SDK `dxc`) |
| `tools/gen_normals.ps1` | after changing a sprite that has a normal map | `assets/sprites/*_normal.png` |

---

## 6. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `clang-cl` / `cmake` "not recognized" | Dev environment not loaded. See §2. |
| `'vswhere.exe' is not recognized` | Harmless. Prepend the VS Installer dir to PATH (§2) to silence it. |
| Link fails with **permission denied** on `SpaceChase.exe` | The game is still running and locks the exe. `Get-Process SpaceChase \| Stop-Process -Force`, then rebuild. |
| D3D12 `E_INVALIDARG` creating a pipeline | Vertex inputs must use `TEXCOORD<n>` semantics where `n` = attribute location (SDL_GPU convention), not `POSITION`. |
| Shader edits have no effect | You didn't re-run `tools/compile_shaders.ps1`. The game loads the committed `.dxil`. |
| Configure fails downloading deps | FetchContent needs internet on first configure. Delete `build/` and retry. |

PowerShell scripting gotchas (when writing asset/tool scripts):
- Variable names are **case-insensitive** (`$H` clobbers `$h`). Use distinct names.
- Don't index 2-D arrays inside method-call arguments. Use flat `[int[]]::new(n)` arrays.

Git gotcha (when committing from a scripted PowerShell, e.g. as an agent): keep commit messages
**plain ASCII with no double quotes or em-dashes**. They can break the command wrapping and leak
words into git as bogus pathspecs.
