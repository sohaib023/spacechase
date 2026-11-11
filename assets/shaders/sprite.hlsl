// Sprite shader (M1, no longer loaded). Hand-compiled to DXIL + SPIR-V by
// tools/compile_shaders.cmake; the blobs are committed under compiled/.
//
// SDL3 GPU resource-binding convention for HLSL (must match exactly):
//   Vertex stage:   textures/samplers -> space0,  uniform buffers -> space1
//   Fragment stage: textures/samplers -> space2,  uniform buffers -> space3
// See SDL_CreateGPUShader docs.

// --- Vertex stage ---------------------------------------------------------
cbuffer CameraUBO : register(b0, space1) {
    float4x4 u_viewProj; // world -> clip
};

// SDL_GPU matches HLSL vertex inputs by TEXCOORD semantic index == attribute
// `location` (NOT by POSITION/etc.). So location 0 -> TEXCOORD0, 1 -> TEXCOORD1.
struct VSInput {
    float2 a_pos : TEXCOORD0;   // attribute location 0: world position
    float2 a_uv  : TEXCOORD1;   // attribute location 1: texcoord
};

struct VSOutput {
    float4 position : SV_Position;
    float2 v_uv     : TEXCOORD0;
};

VSOutput vs_main(VSInput input) {
    VSOutput output;
    output.position = mul(u_viewProj, float4(input.a_pos, 0.0f, 1.0f));
    output.v_uv = input.a_uv;
    return output;
}

// --- Fragment stage -------------------------------------------------------
Texture2D    u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

float4 ps_main(VSOutput input) : SV_Target {
    float4 color = u_texture.Sample(u_sampler, input.v_uv);
    return color;
}
