#pragma once

// Owns GPU textures and hands out stable TextureHandles. Deduplicates by path.
// App owns one; sprites reference textures by handle. The seam where hot-reload
// and an asset-handle system grow later (brief §5).

#include "resources/Texture.hpp"
#include "resources/TextureHandle.hpp"

#include <string>
#include <unordered_map>
#include <vector>

struct SDL_GPUDevice;

namespace sc {

class TextureRegistry {
public:
    // Load (or return the already-loaded handle for) `path`. kInvalidTexture on failure.
    TextureHandle load(SDL_GPUDevice* gpu, const std::string& path);

    bool valid(TextureHandle handle) const { return handle < m_textures.size(); }
    const Texture& get(TextureHandle handle) const { return m_textures[handle]; }

    void destroy_all(SDL_GPUDevice* gpu);

private:
    std::vector<Texture> m_textures;                     // index == handle
    std::unordered_map<std::string, TextureHandle> m_byPath;
};

} // namespace sc
