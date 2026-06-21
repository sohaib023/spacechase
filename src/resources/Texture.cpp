#include "resources/Texture.hpp"

#include "core/Log.hpp"

#include <SDL3/SDL.h>

// Declarations only; the implementation lives in stb_image_impl.cpp (kept in a
// separate TU so its third-party warnings don't fight our -Werror policy).
#include <stb_image.h>

namespace sc {

bool load_texture(SDL_GPUDevice* gpu, const char* path, Texture& out) {
    int w = 0;
    int h = 0;
    int channels = 0;
    // Force 4 channels (RGBA8) so the layout matches the GPU texture format.
    stbi_uc* pixels = stbi_load(path, &w, &h, &channels, 4);
    if (pixels == nullptr) {
        SC_LOG_ERROR("stbi_load failed for '%s': %s", path, stbi_failure_reason());
        return false;
    }
    const Uint32 byteSize = static_cast<Uint32>(w) * static_cast<Uint32>(h) * 4u;

    // Create the destination GPU texture.
    SDL_GPUTextureCreateInfo texInfo{};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texInfo.width = static_cast<Uint32>(w);
    texInfo.height = static_cast<Uint32>(h);
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(gpu, &texInfo);
    if (texture == nullptr) {
        SC_LOG_ERROR("SDL_CreateGPUTexture failed: %s", SDL_GetError());
        stbi_image_free(pixels);
        return false;
    }

    // Staging buffer: CPU-visible memory we copy pixels into, then transfer.
    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = byteSize;
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(gpu, &transferInfo);
    if (transfer == nullptr) {
        SC_LOG_ERROR("SDL_CreateGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTexture(gpu, texture);
        stbi_image_free(pixels);
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(gpu, transfer, false);
    SDL_memcpy(mapped, pixels, byteSize);
    SDL_UnmapGPUTransferBuffer(gpu, transfer);
    stbi_image_free(pixels);

    // Record and submit the upload (texture copy must happen in a copy pass).
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src{};
    src.transfer_buffer = transfer;
    src.offset = 0;
    src.pixels_per_row = static_cast<Uint32>(w);
    src.rows_per_layer = static_cast<Uint32>(h);

    SDL_GPUTextureRegion dst{};
    dst.texture = texture;
    dst.w = static_cast<Uint32>(w);
    dst.h = static_cast<Uint32>(h);
    dst.d = 1;

    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(gpu, transfer);

    out.handle = texture;
    out.width = w;
    out.height = h;
    SC_LOG_INFO("Loaded texture '%s' (%dx%d)", path, w, h);
    return true;
}

void destroy_texture(SDL_GPUDevice* gpu, Texture& tex) {
    if (tex.handle != nullptr) {
        SDL_ReleaseGPUTexture(gpu, tex.handle);
        tex.handle = nullptr;
    }
    tex.width = 0;
    tex.height = 0;
}

} // namespace sc
