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
# M0: SDL3 + Dear ImGui. M1: glm + stb. M2: EnTT + nlohmann_json + doctest.
# Remaining (enet) arrive at the milestone that first needs them (brief §9).
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

# glm — header-only math (vectors/matrices). Brief's chosen math lib; introduced
# at M1 where camera + movement first need it.
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
    GIT_SHALLOW    TRUE
)

# stb — single-header image decoding (stb_image). No upstream CMake; we expose
# the include dir via an INTERFACE target below. Pinned to a commit (no releases).
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4
)

# EnTT — header-only ECS (the entity/component spine). Target: EnTT::EnTT.
FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.15.0
    GIT_SHALLOW    TRUE
)

# nlohmann/json — single-header JSON for definition files. Target: nlohmann_json::nlohmann_json.
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
    GIT_SHALLOW    TRUE
)

# doctest — unit test framework (used by the SpaceChase_tests target). Target: doctest::doctest.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(SDL3 imgui glm stb EnTT nlohmann_json doctest)

# Build the imgui target now that both SDL3 and the imgui sources are available.
include(imgui)

# stb_image: header-only interface target. One .cpp in our tree defines
# STB_IMAGE_IMPLEMENTATION (see src/resources/Texture.cpp).
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

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
    src/resources/Texture.cpp
    src/resources/TextureRegistry.cpp
    src/resources/stb_image_impl.cpp
    src/render/Renderer.cpp
    src/data/DefinitionRegistry.cpp
    src/data/LevelLoader.cpp
    src/physics/Overlap.cpp
    src/physics/SpatialHash.cpp
    src/physics/Raycast.cpp
    src/systems/InputSystem.cpp
    src/systems/MovementSystem.cpp
    src/systems/BeamSystem.cpp
    src/systems/CollisionSystem.cpp
    src/systems/FireSystem.cpp
    src/systems/FloorAltitudeSystem.cpp
    src/systems/ForceFieldSystem.cpp
    src/systems/LifetimeSystem.cpp
    src/systems/LightingSystem.cpp
    src/systems/ProjectileHitSystem.cpp
    src/systems/ProjectileMoveSystem.cpp
    src/systems/RangeSystem.cpp
    src/systems/RenderCollectSystem.cpp
    src/gameplay/Abilities.cpp
    src/gameplay/ProjectileFactory.cpp
    src/gameplay/ShipFactory.cpp
    src/gameplay/Spawning.cpp
    src/gameplay/WallFactory.cpp
)

# The stb_image implementation TU is vendored third-party code; compile it with
# warnings off so its internal warnings don't trip our -Werror policy.
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set_source_files_properties(src/resources/stb_image_impl.cpp
        PROPERTIES COMPILE_OPTIONS "/w")
else()
    set_source_files_properties(src/resources/stb_image_impl.cpp
        PROPERTIES COMPILE_OPTIONS "-w")
endif()

target_include_directories(SpaceChase PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(SpaceChase PRIVATE
    SDL3::SDL3 imgui glm::glm stb EnTT::EnTT nlohmann_json::nlohmann_json)
spacechase_set_warnings(SpaceChase)

# Dev builds load assets straight from the source tree (fast iteration / future
# hot-reload). Shipping builds will instead copy assets next to the exe.
target_compile_definitions(SpaceChase PRIVATE
    SC_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
    # SDL_GPU clip space matches D3D/Vulkan/Metal: depth 0..1 (not GL's -1..1).
    GLM_FORCE_DEPTH_ZERO_TO_ONE)

# On Windows, copy the SDL3 shared library next to the exe so it runs from the
# build dir without PATH fiddling.
if(WIN32)
    add_custom_command(TARGET SpaceChase POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:SpaceChase>
        VERBATIM)
endif()

# ---------------------------------------------------------------------------
# Unit tests (doctest). Single-target policy holds: the test binary compiles the
# pure-logic sources it exercises directly (no shared lib yet). Run via ctest.
# ---------------------------------------------------------------------------
enable_testing()

add_executable(SpaceChase_tests
    tests/test_main.cpp
    tests/test_definitions.cpp
    tests/test_movement.cpp
    tests/test_overlap.cpp
    tests/test_spatial_hash.cpp
    tests/test_collision_resolve.cpp
    tests/test_raycast.cpp
    tests/test_falling.cpp
    tests/test_level.cpp
    tests/test_lights.cpp
    tests/test_forcefield.cpp
    tests/test_fire.cpp
    tests/test_lifetime.cpp
    tests/test_projectile_hit.cpp
    tests/test_abilities.cpp
    tests/test_beam.cpp
    tests/test_range.cpp
    src/data/DefinitionRegistry.cpp   # exercised by test_definitions + test_level
    src/data/LevelLoader.cpp          # exercised by test_level
    src/systems/MovementSystem.cpp    # exercised by test_movement
    src/systems/CollisionSystem.cpp   # exercised by test_collision_resolve
    src/systems/FloorAltitudeSystem.cpp # exercised by test_falling
    src/systems/LightingSystem.cpp    # exercised by test_lights
    src/systems/ForceFieldSystem.cpp  # exercised by test_forcefield
    src/systems/FireSystem.cpp        # exercised by test_fire
    src/systems/LifetimeSystem.cpp    # exercised by test_lifetime
    src/gameplay/Abilities.cpp        # exercised by test_fire
    src/gameplay/ProjectileFactory.cpp # exercised by test_lifetime (commit_deferred)
    src/gameplay/Spawning.cpp         # exercised by test_fire + test_lifetime
    src/systems/ProjectileHitSystem.cpp # exercised by test_projectile_hit
    src/systems/BeamSystem.cpp        # exercised by test_beam
    src/systems/RangeSystem.cpp       # exercised by test_range
    src/systems/ProjectileMoveSystem.cpp # exercised by test_range (bounce + range)
    src/physics/Overlap.cpp           # exercised by test_overlap + collision
    src/physics/SpatialHash.cpp       # exercised by test_spatial_hash + collision
    src/physics/Raycast.cpp           # exercised by test_raycast + projectile_hit
)
target_include_directories(SpaceChase_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
# SDL3 is linked because DefinitionRegistry logs via SDL_Log; glm/EnTT/json for the logic.
target_link_libraries(SpaceChase_tests PRIVATE
    doctest::doctest SDL3::SDL3 glm::glm EnTT::EnTT nlohmann_json::nlohmann_json)
target_compile_definitions(SpaceChase_tests PRIVATE
    SC_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
    GLM_FORCE_DEPTH_ZERO_TO_ONE)
# Note: no -Werror here — doctest's header isn't built to our strict policy.

add_test(NAME unit COMMAND SpaceChase_tests)

if(WIN32)
    add_custom_command(TARGET SpaceChase_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:SpaceChase_tests>
        VERBATIM)
endif()
