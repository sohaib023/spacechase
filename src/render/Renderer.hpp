#pragma once

// 2D sprite batch on the SDL_GPU API. Sprites queued during a frame are built
// into a single dynamic vertex buffer (world-space quads) and drawn in one call.
// M1 assumes a single texture per frame (the ship sheet); multi-texture batching
// comes when projectiles/atlases arrive.

#include "resources/Texture.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <vector>

struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;

namespace sc {

class Renderer {
public:
    bool init(SDL_GPUDevice* gpu, SDL_GPUTextureFormat colorFormat);
    void shutdown();

    // Begin a new frame's batch (clears queued sprites).
    void begin();

    // Queue a sprite in world units. `uvMin`/`uvMax` select a sub-rect of the
    // texture (e.g. a sprite-sheet frame); `rotation` is radians, 0 = upright.
    void draw(const Texture& tex, glm::vec2 pos, glm::vec2 size, float rotation,
              glm::vec2 uvMin = glm::vec2(0.0f), glm::vec2 uvMax = glm::vec2(1.0f));

    // Upload the queued vertices. MUST be called outside a render pass.
    void upload(SDL_GPUCommandBuffer* cmd);

    // Record the batched draw. MUST be called inside an active render pass.
    void render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                const glm::mat4& viewProj);

private:
    struct Vertex {
        float x, y; // world position
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
    SDL_GPUTexture* m_texture = nullptr; // texture for the current batch
};

} // namespace sc
