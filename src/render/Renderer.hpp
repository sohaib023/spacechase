#pragma once

// 2D sprite batch on the SDL_GPU API. Sprites queued during a frame are built
// into a single dynamic vertex buffer (world-space quads). At render time the
// batch flushes one draw call per texture, grouping consecutive same-texture
// sprites — so multiple textures (ship, walls, floor) render in one frame.

#include "resources/TextureHandle.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <vector>

struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;

namespace sc {

class TextureRegistry;

class Renderer {
public:
    bool init(SDL_GPUDevice* gpu, SDL_GPUTextureFormat colorFormat);
    void shutdown();

    // Begin a new frame's batch (clears queued sprites).
    void begin();

    // Queue a sprite in world units (meters). `uvMin`/`uvMax` select a sub-rect
    // of the texture (a sprite-sheet frame); `rotation` radians, 0 = upright.
    void draw(TextureHandle texture, glm::vec2 pos, glm::vec2 size, float rotation,
              glm::vec2 uvMin = glm::vec2(0.0f), glm::vec2 uvMax = glm::vec2(1.0f));

    // Upload queued vertices. MUST be called outside a render pass.
    void upload(SDL_GPUCommandBuffer* cmd);

    // Record the batched draws (one per texture run). MUST be inside a render pass.
    void render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                const glm::mat4& viewProj, const TextureRegistry& textures);

private:
    struct Vertex {
        float x, y; // world position (meters)
        float u, v; // texcoord
    };
    static constexpr int kMaxSprites = 4096;
    static constexpr int kMaxVertices = kMaxSprites * 6;

    SDL_GPUDevice* m_gpu = nullptr;
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    SDL_GPUBuffer* m_vbo = nullptr;
    SDL_GPUTransferBuffer* m_transfer = nullptr;
    std::vector<Vertex> m_vertices;
    std::vector<TextureHandle> m_spriteTextures; // one entry per queued sprite (6 verts)
};

} // namespace sc
