cmake_minimum_required(VERSION 3.24)

project(SpaceChase
    VERSION 0.0.1
    DESCRIPTION "Top-down 2D space shooter on SDL3"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# clangd / tooling reads compile_commands.json from the build dir.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Single output dir so the runnable exe (and its SDL3 DLL on Windows) land together.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(warnings)

# ---------------------------------------------------------------------------
# Dependencies (FetchContent). Pins are deliberately conservative; bump the
# GIT_TAGs to the latest stable releases when you want newer features.
# Only M0 deps are fetched here (SDL3 + Dear ImGui). EnTT/glm/json/enet/doctest
# are added at the milestone that first needs them (see plan / brief §9).
# ---------------------------------------------------------------------------
include(FetchContent)

# SDL3 ---------------------------------------------------------------------
set(SDL_SHARED ON  CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.10  # latest stable SDL3 (3.4.x series)
    GIT_SHALLOW    TRUE
)

# Dear ImGui (no upstream CMakeLists; we build a target in cmake/imgui.cmake) --
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.8         # needs the imgui_impl_sdlgpu3 backend (added in v1.91.6)
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(SDL3 imgui)

# Build the imgui target now that both SDL3 and the imgui sources are available.
include(imgui)

# ---------------------------------------------------------------------------
# Game executable
#
# M0 keeps everything in one executable target. When the engine/game layer
# boundaries start to matter (brief §4, ~M2) split core/render/ecs/data into
# static libraries with enforced dependency direction.
# ---------------------------------------------------------------------------
add_executable(SpaceChase
    src/game/main.cpp
    src/core/App.cpp
)

target_include_directories(SpaceChase PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(SpaceChase PRIVATE SDL3::SDL3 imgui)
spacechase_set_warnings(SpaceChase)

# On Windows, copy the SDL3 shared library next to the exe so it runs from the
# build dir without PATH fiddling.
if(WIN32)
    add_custom_command(TARGET SpaceChase POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:SpaceChase>
        VERBATIM)
endif()
