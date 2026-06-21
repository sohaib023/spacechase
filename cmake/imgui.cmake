# Dear ImGui ships no CMakeLists, so we build a static library target from the
# fetched sources: the core + the SDL3 platform backend + the SDL_GPU renderer
# backend. Consumers just link `imgui`.
#
# Expects `imgui_SOURCE_DIR` to be populated by FetchContent_MakeAvailable and
# the `SDL3::SDL3` target to exist (both done in the root CMakeLists).

if(NOT DEFINED imgui_SOURCE_DIR)
    message(FATAL_ERROR "imgui.cmake included before imgui was fetched")
endif()

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC SDL3::SDL3)

# ImGui is third-party; we do not apply our warnings-as-errors policy to it.
