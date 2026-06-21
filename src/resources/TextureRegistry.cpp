#include "resources/TextureRegistry.hpp"

namespace sc {

TextureHandle TextureRegistry::load(SDL_GPUDevice* gpu, const std::string& path) {
    if (const auto it = m_byPath.find(path); it != m_byPath.end()) {
        return it->second;
    }
    Texture texture;
    if (!load_texture(gpu, path.c_str(), texture)) {
        return kInvalidTexture; // load_texture already logged the reason
    }
    const TextureHandle handle = static_cast<TextureHandle>(m_textures.size());
    m_textures.push_back(texture);
    m_byPath.emplace(path, handle);
    return handle;
}

void TextureRegistry::destroy_all(SDL_GPUDevice* gpu) {
    for (Texture& texture : m_textures) {
        destroy_texture(gpu, texture);
    }
    m_textures.clear();
    m_byPath.clear();
}

} // namespace sc
