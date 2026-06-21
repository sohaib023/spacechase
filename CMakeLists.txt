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

FetchContent_MakeAvailable(SDL3 imgui glm stb)

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
    src/resources/stb_image_impl.cpp
    src/render/Renderer.cpp
    src/gameplay/Ship.cpp
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
target_link_libraries(SpaceChase PRIVATE SDL3::SDL3 imgui glm::glm stb)
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
