#pragma once

// 2D normal-mapped sprite batch on the SDL_GPU API. Sprites queued during a
// frame are built into a dynamic vertex buffer (world-space quads carrying the
// sprite's cos/sin so the fragment shader can rotate sampled normals). At render
// time it flushes one draw per (albedo, normal) texture pair and lights every
// fragment from a 16-light uniform buffer (single-pass forward).

#include "render/LightsUBO.hpp"
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

    void begin();

    // Queue a sprite (meters). `normal` selects the normal map; pass nothing /
    // kInvalidTexture for sprites without one (a flat normal is substituted).
    // `unlit` shows the albedo at full brightness (void background, debug viz).
    void draw(TextureHandle albedo, TextureHandle normal, glm::vec2 pos, glm::vec2 size,
              float rotation, glm::vec2 uvMin = glm::vec2(0.0f), glm::vec2 uvMax = glm::vec2(1.0f),
              bool unlit = false);
    void draw(TextureHandle albedo, glm::vec2 pos, glm::vec2 size, float rotation,
              glm::vec2 uvMin = glm::vec2(0.0f), glm::vec2 uvMax = glm::vec2(1.0f),
              bool unlit = false) {
        draw(albedo, kInvalidTexture, pos, size, rotation, uvMin, uvMax, unlit);
    }

    void upload(SDL_GPUCommandBuffer* cmd); // outside a render pass

    // Inside a render pass. Pushes the camera matrix + the lights UBO, then
    // flushes one draw per texture-pair run.
    void render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass, const glm::mat4& viewProj,
                const TextureRegistry& textures, const LightsUBO& lights);

private:
    struct Vertex {
        float x, y; // world position (meters)
        float u, v; // texcoord
        float c, s; // sprite rotation (cos, sin), for normal rotation
        float e;    // emissive/unlit flag (1 = show albedo as-is)
    };
    struct SpriteTex {
        TextureHandle albedo;
        TextureHandle normal;
    };
    static constexpr int kMaxSprites = 4096;
    static constexpr int kMaxVertices = kMaxSprites * 6;

    SDL_GPUDevice* m_gpu = nullptr;
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    SDL_GPUBuffer* m_vbo = nullptr;
    SDL_GPUTransferBuffer* m_transfer = nullptr;
    SDL_GPUTexture* m_flatNormal = nullptr; // 1x1 (0,0,1) substitute normal
    std::vector<Vertex> m_vertices;
    std::vector<SpriteTex> m_spriteTextures; // one per queued sprite
};

} // namespace sc
