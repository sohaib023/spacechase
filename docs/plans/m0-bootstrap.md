# M0 — Bootstrap

**Status:** ✅ done · **Goal:** a buildable skeleton proving the platform stack end-to-end:
CMake + dependencies, an SDL3 window + GPU device, a cleared screen, Dear ImGui, logging.
No ECS, rendering pipelines or gameplay yet.

## Context

The repo started greenfield (just the brief). Before any gameplay we needed a lean foundation laid
out so every later milestone is an *additive* change, not a restructure.

## Decisions locked

| Decision | Choice | Rationale |
|---|---|---|
| Language | **C++20** | SDL3 GPU + shadercross + EnTT + glm + ImGui's SDL_GPU backend are all C/C++-native and mature. Rust would push us off SDL3 onto wgpu/Bevy (a different architecture). Plain C loses RAII/EnTT/ImGui. |
| Editor | **VS Code** now (CLion possible later) | Both CMake-native, so switching is painless. |
| Windows compiler | **clang-cl** primary, MSVC via preset | Clang diagnostics consistent across platforms + MSVC ABI + PDBs, so VS Code's `cppvsdbg` debugs it. Best of both. |
| Build | **CMake ≥ 3.24 + FetchContent + Ninja (Multi-Config) + CMakePresets.json** | Pinned deps; presets make compiler switching one command. |
| Logging | **Thin macros over `SDL_Log*`** | SDL3 already has categorized, prioritized logging. No spdlog (anti-bloat). |
| M0 deps | **SDL3 `release-3.4.10` + Dear ImGui `v1.92.8` only** | Others are added at the milestone that first needs them. ImGui's `imgui_impl_sdlgpu3` backend requires ≥ v1.91.6. |

Prerequisites/installation are documented in [`../getting-started.md`](../getting-started.md).

## Files created

```
CMakeLists.txt, CMakePresets.json, .gitignore, .clang-format, .editorconfig
.vscode/{settings,launch,extensions}.json
cmake/imgui.cmake      builds an `imgui` static lib: core + imgui_impl_sdl3 + imgui_impl_sdlgpu3
cmake/warnings.cmake   per-compiler strict warnings, -Werror on our targets only
src/core/App.{hpp,cpp} window + GPU device; init/run/shutdown; the loop
src/core/Log.hpp       logging macros
src/game/main.cpp      entry point
src/<module>/.gitkeep, assets/<kind>/.gitkeep, tests/, docs/   layout from brief §4 made visible
```

## Steps

1. Directory skeleton + ignore/format files.
2. Root `CMakeLists.txt`: C++20, `CMAKE_EXPORT_COMPILE_COMMANDS`, FetchContent SDL3 + ImGui, one
   `SpaceChase` executable (library split deferred until boundaries matter).
3. `cmake/imgui.cmake` (ImGui ships no CMakeLists).
4. `Log.hpp`.
5. `App`: `SDL_Init` → window → `SDL_CreateGPUDevice` → claim window. Loop: events → ImGui frame →
   demo window → acquire command buffer + swapchain → clear pass → ImGui two-stage submit
   (`PrepareDrawData` before the pass, `RenderDrawData` inside). Reverse-order shutdown.
6. `main.cpp`.
7. Presets: `windows-clang-cl` (default), `windows-msvc`, Linux/macOS stubs; Debug/Release.
8. `.vscode/`: clangd for IntelliSense, cpptools for debugging only, CMake Tools on presets.

## Verification

Clean `-Werror` build with the clang-cl preset; window shows the clear color + a responsive ImGui
demo; F5 breakpoint in `App::run` hits with readable locals; the MSVC preset also builds;
`compile_commands.json` generated and clangd happy.

## Outcome

Delivered as planned. Lesson: the toolchain isn't on a plain PowerShell PATH (developer environment
needed), and the noisy-but-harmless `vswhere.exe` message. Both are documented in getting-started.
