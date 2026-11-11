# Compiles assets/shaders/*.hlsl to DXIL (D3D12, Windows) and SPIR-V (Vulkan,
# Linux). Run by hand after editing a shader, then commit assets/shaders/compiled/:
#   cmake -P tools/compile_shaders.cmake
# dxc lookup: DXC=<path> override, .tools/dxc (tools/setup_ubuntu.sh), PATH, then
# the Windows SDK. SPIR-V needs a dxc built with SPIR-V support: the GitHub
# release builds are, the Windows SDK copy is not.
cmake_minimum_required(VERSION 3.24)

get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(src_dir "${root}/assets/shaders")
set(out_dir "${src_dir}/compiled")

if(NOT DXC)
    find_program(DXC NAMES dxc PATHS "${root}/.tools/dxc/bin" NO_DEFAULT_PATH)
endif()
if(NOT DXC)
    find_program(DXC NAMES dxc)
endif()
if(NOT DXC)
    file(GLOB sdk_dxc "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/dxc.exe")
    if(sdk_dxc)
        list(SORT sdk_dxc)
        list(GET sdk_dxc -1 DXC)
    endif()
endif()
if(NOT DXC)
    message(FATAL_ERROR "dxc not found (run tools/setup_ubuntu.sh, or install the Windows SDK)")
endif()
message(STATUS "Using dxc: ${DXC}")
file(MAKE_DIRECTORY "${out_dir}")

# SPIR-V maps register(xN, spaceM) to set M / binding N, which is the SDL_GPU
# Vulkan layout. Vertex inputs/outputs get locations in declaration order, so
# TEXCOORD<n> must be declared in order n = 0, 1, 2...
set(formats dxil spv)
set(dxil_flags "")
set(spv_flags -spirv -fspv-target-env=vulkan1.0)

foreach(fmt IN LISTS formats)
    foreach(shader sprite lit_sprite)
        foreach(stage vert frag)
            if(stage STREQUAL "vert")
                set(entry vs_main)
                set(profile vs_6_0)
            else()
                set(entry ps_main)
                set(profile ps_6_0)
            endif()
            set(out "${out_dir}/${shader}.${stage}.${fmt}")
            execute_process(
                COMMAND "${DXC}" -T ${profile} -E ${entry} ${${fmt}_flags}
                        -Fo "${out}" "${src_dir}/${shader}.hlsl"
                RESULT_VARIABLE rc)
            if(NOT rc EQUAL 0)
                set(hint "")
                if(fmt STREQUAL "spv")
                    set(hint " (if this dxc lacks SPIR-V support, put a GitHub release dxc in .tools/dxc/bin)")
                endif()
                message(FATAL_ERROR "dxc failed: ${shader}.hlsl ${entry} -> ${fmt}${hint}")
            endif()
            message(STATUS "compiled ${shader}.hlsl:${entry} -> ${shader}.${stage}.${fmt}")
        endforeach()
    endforeach()
endforeach()
