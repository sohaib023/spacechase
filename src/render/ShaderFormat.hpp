#pragma once

// Compiled-shader formats we ship (tools/compile_shaders.cmake). The device is
// created offering all of them; SDL picks the backend (D3D12 before Vulkan) and
// the renderer loads the matching blobs.

#include <SDL3/SDL_gpu.h>

namespace sc {

inline constexpr SDL_GPUShaderFormat kShippedShaderFormats =
    SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV;

struct ShaderBlobFormat {
    SDL_GPUShaderFormat format;
    const char* extension; // file suffix in assets/shaders/compiled/
};

// The blob format to load on a device created with kShippedShaderFormats.
inline ShaderBlobFormat shader_blob_format(SDL_GPUDevice* gpu) {
    if ((SDL_GetGPUShaderFormats(gpu) & SDL_GPU_SHADERFORMAT_DXIL) != 0) {
        return {SDL_GPU_SHADERFORMAT_DXIL, ".dxil"};
    }
    return {SDL_GPU_SHADERFORMAT_SPIRV, ".spv"};
}

} // namespace sc
