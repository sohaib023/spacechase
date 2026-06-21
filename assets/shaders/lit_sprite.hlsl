// Normal-mapped 2.5D sprite lighting (M4). Authored once in HLSL, hand-compiled
// to DXIL for now (see tools/compile_shaders.ps1); feeds SDL_shadercross later
// unchanged. SDL_GPU register conventions: VS uniforms space1; FS textures/
// samplers space2; FS uniforms space3. Vertex inputs are TEXCOORD<n> == location.
//
// Lighting is done in WORLD space (meters). The sprite plane is z=0; lights sit
// at a height z above it (the key knob). The sampled tangent-space normal is
// rotated by the sprite's rotation so lighting stays world-consistent as the
// ship turns (brief §6.7). View direction is straight down (+Z), top-down camera.

// --- Vertex stage ---------------------------------------------------------
cbuffer CameraUBO : register(b0, space1) {
    float4x4 u_viewProj;
};

struct VSInput {
    float2 a_pos : TEXCOORD0; // world position (rotation already baked in by CPU)
    float2 a_uv  : TEXCOORD1;
    float2 a_rot : TEXCOORD2; // (cos, sin) of the sprite's rotation
    float  a_emissive : TEXCOORD3; // 1 = unlit (show albedo as-is), 0 = lit
};

struct VSOutput {
    float4 position  : SV_Position;
    float2 v_uv      : TEXCOORD0;
    float2 v_world   : TEXCOORD1; // world xy (meters)
    float2 v_rot     : TEXCOORD2; // (cos, sin), constant across the quad
    float  v_emissive : TEXCOORD3;
};

VSOutput vs_main(VSInput input) {
    VSOutput output;
    output.position = mul(u_viewProj, float4(input.a_pos, 0.0f, 1.0f));
    output.v_uv = input.a_uv;
    output.v_world = input.a_pos;
    output.v_rot = input.a_rot;
    output.v_emissive = input.a_emissive;
    return output;
}

// --- Fragment stage -------------------------------------------------------
Texture2D    u_albedo  : register(t0, space2);
Texture2D    u_normal  : register(t1, space2);
SamplerState u_sampler : register(s0, space2);

#define SC_MAX_LIGHTS 16

struct Light {
    float4 posHeight; // xy = world pos, z = height, w = type (0 point, 1 directional)
    float4 color;     // rgb = color, a = intensity
    float4 rangeDir;  // x = range, yz = direction (directional), w = unused
};

cbuffer LightsUBO : register(b0, space3) {
    float4 u_ambient; // rgb = color, a = strength
    float4 u_params;  // x = shininess, y = specular strength
    int4   u_counts;  // x = active light count
    Light  u_lights[SC_MAX_LIGHTS];
};

float4 ps_main(VSOutput input) : SV_Target {
    float4 albedo = u_albedo.Sample(u_sampler, input.v_uv);

    // Unlit sprites (e.g. the void background, "off into infinity") are shown
    // at full albedo, untouched by lights/ambient.
    if (input.v_emissive > 0.5f) {
        return albedo;
    }

    // Unpack the tangent-space normal and rotate its in-plane part by the
    // sprite rotation (SAME cos/sin the CPU baked into the vertex positions).
    float3 nTex = u_normal.Sample(u_sampler, input.v_uv).xyz * 2.0f - 1.0f;
    float c = input.v_rot.x;
    float s = input.v_rot.y;
    float2 nr = float2(nTex.x * c - nTex.y * s, nTex.x * s + nTex.y * c);
    float3 N = normalize(float3(nr, max(nTex.z, 1e-3f)));

    float3 P = float3(input.v_world, 0.0f);
    float3 V = float3(0.0f, 0.0f, 1.0f); // top-down view

    float3 lit = albedo.rgb * (u_ambient.rgb * u_ambient.a);

    int count = min(u_counts.x, SC_MAX_LIGHTS);
    for (int i = 0; i < count; ++i) {
        Light light = u_lights[i];
        float3 Ldir;
        float atten;
        if (light.posHeight.w < 0.5f) {
            // Point light: direction + distance from the fragment to the light.
            float3 toL = float3(light.posHeight.xy - P.xy, light.posHeight.z);
            float dist = length(toL);
            Ldir = toL / max(dist, 1e-4f);
            float range = max(light.rangeDir.x, 1e-3f);
            atten = saturate(1.0f - dist / range);
        } else {
            // Directional: fixed in-plane direction tilted up by the height.
            Ldir = normalize(float3(light.rangeDir.yz, light.posHeight.z));
            atten = 1.0f;
        }

        float ndl = saturate(dot(N, Ldir));
        float3 radiance = light.color.rgb * (light.color.a * atten);

        // Diffuse.
        lit += albedo.rgb * radiance * ndl;

        // Blinn-Phong specular (gated by N·L so back faces don't shine).
        float3 H = normalize(Ldir + V);
        float spec = pow(saturate(dot(N, H)), max(u_params.x, 1.0f)) * u_params.y;
        lit += radiance * spec * ndl;
    }

    return float4(lit, albedo.a);
}
