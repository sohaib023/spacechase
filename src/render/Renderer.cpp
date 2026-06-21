#include "render/Renderer.hpp"

#include "core/Log.hpp"
#include "resources/TextureRegistry.hpp"

#include <SDL3/SDL.h>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstddef>
#include <string>

namespace sc {

namespace {

SDL_GPUShader* load_shader(SDL_GPUDevice* gpu, const std::string& path,
                           SDL_GPUShaderStage stage, const char* entrypoint,
                           Uint32 numSamplers, Uint32 numUniformBuffers) {
    size_t codeSize = 0;
    void* code = SDL_LoadFile(path.c_str(), &codeSize);
    if (code == nullptr) {
        SC_LOG_ERROR("Failed to load shader '%s': %s", path.c_str(), SDL_GetError());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};
    info.code = static_cast<const Uint8*>(code);
    info.code_size = codeSize;
    info.entrypoint = entrypoint;
    info.format = SDL_GPU_SHADERFORMAT_DXIL;
    info.stage = stage;
    info.num_samplers = numSamplers;
    info.num_uniform_buffers = numUniformBuffers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(gpu, &info);
    SDL_free(code);
    if (shader == nullptr) {
        SC_LOG_ERROR("SDL_CreateGPUShader '%s': %s", path.c_str(), SDL_GetError());
    }
    return shader;
}

} // namespace

bool Renderer::init(SDL_GPUDevice* gpu, SDL_GPUTextureFormat colorFormat) {
    m_gpu = gpu;
    m_vertices.reserve(kMaxVertices);

    const std::string shaderDir = std::string(SC_ASSET_DIR) + "/shaders/compiled/";
    // Vertex: 1 uniform buffer (camera). Fragment: 2 samplers (albedo + normal),
    // 1 uniform buffer (lights).
    SDL_GPUShader* vs = load_shader(gpu, shaderDir + "lit_sprite.vert.dxil",
                                    SDL_GPU_SHADERSTAGE_VERTEX, "vs_main", 0, 1);
    SDL_GPUShader* ps = load_shader(gpu, shaderDir + "lit_sprite.frag.dxil",
                                    SDL_GPU_SHADERSTAGE_FRAGMENT, "ps_main", 2, 1);
    if (vs == nullptr || ps == nullptr) {
        if (vs) SDL_ReleaseGPUShader(gpu, vs);
        if (ps) SDL_ReleaseGPUShader(gpu, ps);
        return false;
    }

    SDL_GPUVertexBufferDescription vbDesc{};
    vbDesc.slot = 0;
    vbDesc.pitch = sizeof(Vertex);
    vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[4]{};
    attrs[0].location = 0; // TEXCOORD0 -> position
    attrs[0].buffer_slot = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[0].offset = 0;
    attrs[1].location = 1; // TEXCOORD1 -> uv
    attrs[1].buffer_slot = 0;
    attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[1].offset = sizeof(float) * 2;
    attrs[2].location = 2; // TEXCOORD2 -> (cos, sin) of sprite rotation
    attrs[2].buffer_slot = 0;
    attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[2].offset = sizeof(float) * 4;
    attrs[3].location = 3; // TEXCOORD3 -> emissive/unlit flag
    attrs[3].buffer_slot = 0;
    attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    attrs[3].offset = sizeof(float) * 6;

    SDL_GPUVertexInputState vertexInput{};
    vertexInput.vertex_buffer_descriptions = &vbDesc;
    vertexInput.num_vertex_buffers = 1;
    vertexInput.vertex_attributes = attrs;
    vertexInput.num_vertex_attributes = 4;

    SDL_GPUColorTargetBlendState blend{};
    blend.enable_blend = true;
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = colorFormat;
    colorTarget.blend_state = blend;

    SDL_GPUGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.vertex_shader = vs;
    pipeInfo.fragment_shader = ps;
    pipeInfo.vertex_input_state = vertexInput;
    pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeInfo.target_info.color_target_descriptions = &colorTarget;
    pipeInfo.target_info.num_color_targets = 1;

    m_pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeInfo);
    SDL_ReleaseGPUShader(gpu, vs);
    SDL_ReleaseGPUShader(gpu, ps);
    if (m_pipeline == nullptr) {
        SC_LOG_ERROR("SDL_CreateGPUGraphicsPipeline failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUSamplerCreateInfo samplerInfo{};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    m_sampler = SDL_CreateGPUSampler(gpu, &samplerInfo);
    if (m_sampler == nullptr) {
        SC_LOG_ERROR("SDL_CreateGPUSampler failed: %s", SDL_GetError());
        return false;
    }

    // Dynamic vertex buffer + a persistent upload staging buffer, both sized for
    // the per-frame maximum. Re-uploaded each frame in upload().
    const Uint32 bufferSize = static_cast<Uint32>(kMaxVertices) * sizeof(Vertex);
    SDL_GPUBufferCreateInfo bufInfo{};
    bufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufInfo.size = bufferSize;
    m_vbo = SDL_CreateGPUBuffer(gpu, &bufInfo);

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = bufferSize;
    m_transfer = SDL_CreateGPUTransferBuffer(gpu, &transferInfo);
    if (m_vbo == nullptr || m_transfer == nullptr) {
        SC_LOG_ERROR("Failed to create sprite batch buffers: %s", SDL_GetError());
        return false;
    }

    // 1x1 "flat" normal map (0,0,1) bound at slot 1 for sprites without one, so
    // the 2-sampler pipeline always has both textures bound.
    {
        SDL_GPUTextureCreateInfo texInfo{};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texInfo.width = 1;
        texInfo.height = 1;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        m_flatNormal = SDL_CreateGPUTexture(gpu, &texInfo);

        const Uint8 flat[4] = {128, 128, 255, 255}; // encodes (0,0,1)
        SDL_GPUTransferBufferCreateInfo tbi{};
        tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbi.size = sizeof(flat);
        SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(gpu, &tbi);
        SDL_memcpy(SDL_MapGPUTransferBuffer(gpu, tb, false), flat, sizeof(flat));
        SDL_UnmapGPUTransferBuffer(gpu, tb);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = tb;
        src.pixels_per_row = 1;
        src.rows_per_layer = 1;
        SDL_GPUTextureRegion dst{};
        dst.texture = m_flatNormal;
        dst.w = 1;
        dst.h = 1;
        dst.d = 1;
        SDL_UploadToGPUTexture(copyPass, &src, &dst, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(gpu, tb);
    }
    if (m_flatNormal == nullptr) {
        SC_LOG_ERROR("Failed to create flat-normal texture: %s", SDL_GetError());
        return false;
    }

    SC_LOG_INFO("Renderer initialized (lit sprite batch, max %d sprites).", kMaxSprites);
    return true;
}

void Renderer::begin() {
    m_vertices.clear();
    m_spriteTextures.clear();
}

void Renderer::draw(TextureHandle albedo, TextureHandle normal, glm::vec2 pos, glm::vec2 size,
                    float rotation, glm::vec2 uvMin, glm::vec2 uvMax, bool unlit) {
    if (albedo == kInvalidTexture) {
        return;
    }
    if (static_cast<int>(m_vertices.size()) + 6 > kMaxVertices) {
        return; // batch full; silently drop (budget is generous)
    }
    m_spriteTextures.push_back(SpriteTex{albedo, normal});

    const float c = std::cos(rotation);
    const float s = std::sin(rotation);
    const float e = unlit ? 1.0f : 0.0f;
    const glm::vec2 h = size * 0.5f;
    auto push = [&](float lx, float ly, float u, float v) {
        m_vertices.push_back(
            {lx * c - ly * s + pos.x, lx * s + ly * c + pos.y, u, v, c, s, e});
    };
    // Two triangles (CCW): TL, TR, BR and TL, BR, BL. UV origin top-left.
    push(-h.x, h.y, uvMin.x, uvMin.y);
    push(h.x, h.y, uvMax.x, uvMin.y);
    push(h.x, -h.y, uvMax.x, uvMax.y);
    push(-h.x, h.y, uvMin.x, uvMin.y);
    push(h.x, -h.y, uvMax.x, uvMax.y);
    push(-h.x, -h.y, uvMin.x, uvMax.y);
}

void Renderer::upload(SDL_GPUCommandBuffer* cmd) {
    if (m_vertices.empty()) {
        return;
    }
    const Uint32 bytes = static_cast<Uint32>(m_vertices.size()) * sizeof(Vertex);

    void* mapped = SDL_MapGPUTransferBuffer(m_gpu, m_transfer, true);
    SDL_memcpy(mapped, m_vertices.data(), bytes);
    SDL_UnmapGPUTransferBuffer(m_gpu, m_transfer);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src{};
    src.transfer_buffer = m_transfer;
    src.offset = 0;
    SDL_GPUBufferRegion dst{};
    dst.buffer = m_vbo;
    dst.offset = 0;
    dst.size = bytes;
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, true);
    SDL_EndGPUCopyPass(copyPass);
}

void Renderer::render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                      const glm::mat4& viewProj, const TextureRegistry& textures,
                      const LightsUBO& lights) {
    if (m_spriteTextures.empty()) {
        return;
    }

    // Pipeline, vertex buffer, camera matrix, and lights are bound once.
    SDL_BindGPUGraphicsPipeline(pass, m_pipeline);
    SDL_GPUBufferBinding vbBinding{};
    vbBinding.buffer = m_vbo;
    vbBinding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &vbBinding, 1);
    SDL_PushGPUVertexUniformData(cmd, 0, glm::value_ptr(viewProj), sizeof(viewProj));
    SDL_PushGPUFragmentUniformData(cmd, 0, &lights, sizeof(lights));

    auto same = [](const SpriteTex& a, const SpriteTex& b) {
        return a.albedo == b.albedo && a.normal == b.normal;
    };
    // Walk the queued sprites, flushing one draw per (albedo, normal) run.
    std::size_t i = 0;
    while (i < m_spriteTextures.size()) {
        const SpriteTex tex = m_spriteTextures[i];
        std::size_t j = i;
        while (j < m_spriteTextures.size() && same(m_spriteTextures[j], tex)) {
            ++j;
        }
        if (textures.valid(tex.albedo)) {
            SDL_GPUTextureSamplerBinding binds[2]{};
            binds[0].texture = textures.get(tex.albedo).handle;
            binds[0].sampler = m_sampler;
            binds[1].texture =
                textures.valid(tex.normal) ? textures.get(tex.normal).handle : m_flatNormal;
            binds[1].sampler = m_sampler;
            SDL_BindGPUFragmentSamplers(pass, 0, binds, 2);
            SDL_DrawGPUPrimitives(pass, static_cast<Uint32>((j - i) * 6), 1,
                                  static_cast<Uint32>(i * 6), 0);
        }
        i = j;
    }
}

void Renderer::shutdown() {
    if (m_gpu == nullptr) {
        return;
    }
    if (m_flatNormal != nullptr) {
        SDL_ReleaseGPUTexture(m_gpu, m_flatNormal);
        m_flatNormal = nullptr;
    }
    if (m_transfer != nullptr) {
        SDL_ReleaseGPUTransferBuffer(m_gpu, m_transfer);
        m_transfer = nullptr;
    }
    if (m_vbo != nullptr) {
        SDL_ReleaseGPUBuffer(m_gpu, m_vbo);
        m_vbo = nullptr;
    }
    if (m_sampler != nullptr) {
        SDL_ReleaseGPUSampler(m_gpu, m_sampler);
        m_sampler = nullptr;
    }
    if (m_pipeline != nullptr) {
        SDL_ReleaseGPUGraphicsPipeline(m_gpu, m_pipeline);
        m_pipeline = nullptr;
    }
    m_gpu = nullptr;
}

} // namespace sc
