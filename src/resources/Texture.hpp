#pragma once

// GPU texture loaded from an image file. M1: decoded with stb_image and
// uploaded to an SDL_GPU sampler texture. Lives in `resources` (brief §5);
// the handle registry + hot-reload come later.

struct SDL_GPUDevice;
struct SDL_GPUTexture;

namespace sc {

struct Texture {
    SDL_GPUTexture* handle = nullptr;
    int width = 0;
    int height = 0;
};

// Decode `path` (RGBA) and upload to a GPU texture. Returns false (and logs) on
// failure. The path is a full filesystem path (see SC_ASSET_DIR).
bool load_texture(SDL_GPUDevice* gpu, const char* path, Texture& out);

// Release the GPU texture. Safe on a zero-initialized Texture.
void destroy_texture(SDL_GPUDevice* gpu, Texture& tex);

} // namespace sc
