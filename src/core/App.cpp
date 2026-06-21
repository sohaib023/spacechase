#include "core/App.hpp"

#include "components/Altitude.hpp"
#include "components/CollisionLayer.hpp"
#include "components/ShipStats.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "core/Log.hpp"
#include "data/ShipDef.hpp"
#include "gameplay/ShipFactory.hpp"
#include "gameplay/WallFactory.hpp"
#include "physics/Raycast.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/FloorAltitudeSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/RenderCollectSystem.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

#include <cmath>
#include <string>

namespace sc {

namespace {
constexpr int kDefaultWidth = 1280;
constexpr int kDefaultHeight = 720;
// Background clear color (dark blue-grey) — proves the render-pass clear works.
constexpr SDL_FColor kClearColor{0.10f, 0.10f, 0.12f, 1.0f};
} // namespace

bool App::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SC_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    m_window = SDL_CreateWindow("Space Chase", kDefaultWidth, kDefaultHeight,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (m_window == nullptr) {
        SC_LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    // M1 staged shaders are precompiled to DXIL, so request DXIL only — this
    // deterministically selects the D3D12 backend and matches our shader blobs
    // (ImGui's SDL_GPU backend has DXIL shaders too). The SDL_shadercross build
    // step will later emit all formats and we can request them all again.
    m_gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL,
                                /*debug_mode=*/true, /*name=*/nullptr);
    if (m_gpu == nullptr) {
        SC_LOG_ERROR("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(m_gpu, m_window)) {
        SC_LOG_ERROR("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return false;
    }

    SC_LOG_INFO("GPU device driver: %s", SDL_GetGPUDeviceDriver(m_gpu));

    // --- Dear ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForSDLGPU(m_window)) {
        SC_LOG_ERROR("ImGui_ImplSDL3_InitForSDLGPU failed");
        return false;
    }

    ImGui_ImplSDLGPU3_InitInfo initInfo{};
    initInfo.Device = m_gpu;
    initInfo.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(m_gpu, m_window);
    initInfo.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    if (!ImGui_ImplSDLGPU3_Init(&initInfo)) {
        SC_LOG_ERROR("ImGui_ImplSDLGPU3_Init failed");
        return false;
    }
    m_imguiInitialized = true;

    // --- Renderer + content ---
    const SDL_GPUTextureFormat colorFormat = SDL_GetGPUSwapchainTextureFormat(m_gpu, m_window);
    if (!m_renderer.init(m_gpu, colorFormat)) {
        return false;
    }

    // --- Content: load definitions, then spawn the player ship from data ---
    const std::string defsDir = std::string(SC_ASSET_DIR) + "/definitions/ships";
    if (!m_defs.load_ships_from_dir(defsDir)) {
        SC_LOG_ERROR("No ship definitions loaded from %s", defsDir.c_str());
        return false;
    }
    const ShipDef* playerDef = m_defs.find_ship("player_ship");
    if (playerDef == nullptr) {
        SC_LOG_ERROR("Missing 'player_ship' definition");
        return false;
    }

    const std::string shipPath = std::string(SC_ASSET_DIR) + "/" + playerDef->spriteSheet;
    const TextureHandle shipTexture = m_textures.load(m_gpu, shipPath);
    if (shipTexture == kInvalidTexture) {
        return false;
    }

    // Test scene: a few walls to collide with (in-code for M3.4; replaced by a
    // JSON level in M3.7). Spawned before the player so the ship draws on top.
    const TextureHandle wallTexture =
        m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/sprites/wall.png");
    if (wallTexture != kInvalidTexture) {
        spawn_wall(m_world.registry, wallTexture, {8.0f, 0.0f}, {1.0f, 4.0f}, false);
        spawn_wall(m_world.registry, wallTexture, {-8.0f, 0.0f}, {1.0f, 4.0f}, false);
        spawn_wall(m_world.registry, wallTexture, {0.0f, 6.0f}, {6.0f, 1.0f}, false);
        spawn_wall(m_world.registry, wallTexture, {3.0f, -5.0f}, {1.5f, 1.5f}, true);
    }

    m_world.playerShip = spawn_ship(m_world.registry, *playerDef, shipTexture,
                                    glm::vec2(0.0f, 0.0f), /*facing=*/0.0f,
                                    /*playerControlled=*/true);

    m_debugTexture = m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/sprites/debug_white.png");

    // Floor: a big platform (continuous meters) with a couple of void pits to
    // fall into. (In-code for M3.6; a JSON level replaces it in M3.7.)
    m_floorTexture = m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/sprites/floor.png");
    m_voidTexture = m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/sprites/void.png");
    m_level.floorRects.push_back(AABB{{0.0f, 0.0f}, {16.0f, 12.0f}});
    m_level.voidRects.push_back(AABB{{-6.0f, -5.0f}, {2.0f, 2.0f}});
    m_level.voidRects.push_back(AABB{{7.0f, 4.0f}, {1.5f, 1.5f}});

    SC_LOG_INFO("Space Chase initialized.");
    return true;
}

void App::run() {
    // Fixed-timestep simulation decoupled from rendering (brief §3.4): the sim
    // advances in 60 Hz steps; the render interpolates between the last two
    // states by `alpha` so motion stays smooth regardless of frame rate.
    constexpr float kFixedDt = 1.0f / 60.0f;
    const double perfFreq = static_cast<double>(SDL_GetPerformanceFrequency());
    Uint64 prevCounter = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    InputActions actions; // most recent sampled actions (also drives tilt frame)

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                       event.window.windowID == SDL_GetWindowID(m_window)) {
                running = false;
            }
        }

        // --- Timing ---
        const Uint64 nowCounter = SDL_GetPerformanceCounter();
        double frameTime = static_cast<double>(nowCounter - prevCounter) / perfFreq;
        prevCounter = nowCounter;
        if (frameTime > 0.25) {
            frameTime = 0.25; // clamp to avoid a spiral of death after a stall
        }
        accumulator += frameTime;

        // --- Input → abstract actions (skip while ImGui owns the keyboard) ---
        actions = InputActions{};
        if (!ImGui::GetIO().WantCaptureKeyboard) {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) actions.thrust += 1.0f;
            if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) actions.thrust -= 1.0f;
            if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) actions.turn += 1.0f;
            if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) actions.turn -= 1.0f;
        }

        // --- Fixed-step simulation pipeline ---
        while (accumulator >= kFixedDt) {
            snapshot_prev_transforms(m_world.registry);
            input_apply(m_world.registry, actions);
            apply_fall_control(m_world.registry, m_fallConfig); // lock thrust if falling
            movement_step(m_world.registry, kFixedDt);
            collision_step(m_world.registry, kFixedDt);
            floor_altitude_step(m_world.registry, m_level, m_fallConfig, m_spawn, kFixedDt);
            accumulator -= kFixedDt;
        }
        const float alpha = static_cast<float>(accumulator / kFixedDt);

        // --- UI ---
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("Ship")) {
            Registry& reg = m_world.registry;
            const Entity player = m_world.playerShip;
            if (reg.valid(player)) {
                const Transform& transform = reg.get<Transform>(player);
                const Velocity& velocity = reg.get<Velocity>(player);
                ShipStats& stats = reg.get<ShipStats>(player);
                ImGui::Text("pos    (%.1f, %.1f) m", transform.position.x, transform.position.y);
                ImGui::Text("vel    (%.1f, %.1f) m/s", velocity.value.x, velocity.value.y);
                ImGui::Text("speed  %.1f m/s", glm::length(velocity.value));
                ImGui::Text("facing %.2f rad", transform.rotation);
                if (const Altitude* altitude = reg.try_get<Altitude>(player)) {
                    ImGui::Text("alt    %.2f m %s", altitude->z, altitude->falling ? "(falling)" : "");
                }
                ImGui::Separator();
                ImGui::SliderFloat("acceleration", &stats.acceleration, 5.0f, 200.0f, "%.1f m/s2");
                ImGui::SliderFloat("max speed", &stats.maxSpeed, 2.0f, 60.0f, "%.1f m/s");
                ImGui::SliderFloat("turn speed", &stats.turnSpeed, 0.5f, 8.0f, "%.2f rad/s");
                ImGui::SliderFloat("drag", &stats.drag, 0.0f, 5.0f);
                ImGui::Checkbox("debug ray", &m_showDebugRay);
                ImGui::Checkbox("control while falling", &m_fallConfig.controlWhileFalling);
                ImGui::TextDisabled("WASD / arrow keys to fly");
            }
        }
        ImGui::End();
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        // --- Queue the world into the sprite batch (floor → void → entities) ---
        m_renderer.begin();
        for (const AABB& floorRect : m_level.floorRects) {
            m_renderer.draw(m_floorTexture, floorRect.center, floorRect.halfExtents * 2.0f, 0.0f);
        }
        for (const AABB& voidRect : m_level.voidRects) {
            m_renderer.draw(m_voidTexture, voidRect.center, voidRect.halfExtents * 2.0f, 0.0f);
        }
        render_collect(m_world.registry, m_renderer, alpha);

        // Debug ray: cast from the player along its facing, drawn as a thin beam
        // plus a marker at the hit point. Proves the raycast API end-to-end.
        if (m_showDebugRay && m_debugTexture != kInvalidTexture &&
            m_world.registry.valid(m_world.playerShip)) {
            const Transform& pt = m_world.registry.get<Transform>(m_world.playerShip);
            const glm::vec2 dir{-std::sin(pt.rotation), std::cos(pt.rotation)};
            constexpr float kMaxDist = 50.0f;
            const RayHit hit = raycast(m_world.registry, pt.position, dir, kMaxDist, layer::Wall);
            const float len = hit.hit ? hit.t : kMaxDist;
            const glm::vec2 mid = pt.position + dir * (len * 0.5f);
            const float phi = std::atan2(dir.y, dir.x);
            m_renderer.draw(m_debugTexture, mid, glm::vec2(len, 0.06f), phi);
            if (hit.hit) {
                m_renderer.draw(m_debugTexture, hit.point, glm::vec2(0.4f, 0.4f), 0.0f);
            }
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_gpu);
        if (cmd == nullptr) {
            SC_LOG_ERROR("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
            continue;
        }

        SDL_GPUTexture* swapchain = nullptr;
        if (!SDL_AcquireGPUSwapchainTexture(cmd, m_window, &swapchain, nullptr, nullptr)) {
            SC_LOG_ERROR("SDL_AcquireGPUSwapchainTexture failed: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(cmd);
            continue;
        }

        // swapchain can be null if the window is minimized/occluded — skip drawing.
        if (swapchain != nullptr) {
            // All GPU uploads (sprite vertices, ImGui draw data) must happen
            // before the render pass begins.
            m_renderer.upload(cmd);
            ImGui_ImplSDLGPU3_PrepareDrawData(drawData, cmd);

            int pxW = 0;
            int pxH = 0;
            SDL_GetWindowSizeInPixels(m_window, &pxW, &pxH);
            const glm::mat4 viewProj =
                m_camera.view_proj(static_cast<float>(pxW), static_cast<float>(pxH));

            SDL_GPUColorTargetInfo target{};
            target.texture = swapchain;
            target.clear_color = kClearColor;
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
            m_renderer.render(cmd, pass, viewProj, m_textures);
            ImGui_ImplSDLGPU3_RenderDrawData(drawData, cmd, pass);
            SDL_EndGPURenderPass(pass);
        }

        SDL_SubmitGPUCommandBuffer(cmd);
    }
}

void App::shutdown() {
    if (m_gpu != nullptr) {
        // Make sure the GPU is idle before destroying resources it may still use.
        SDL_WaitForGPUIdle(m_gpu);
        m_textures.destroy_all(m_gpu);
        m_renderer.shutdown();
    }

    if (m_imguiInitialized) {
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        m_imguiInitialized = false;
    }

    if (m_gpu != nullptr && m_window != nullptr) {
        SDL_ReleaseWindowFromGPUDevice(m_gpu, m_window);
    }
    if (m_gpu != nullptr) {
        SDL_DestroyGPUDevice(m_gpu);
        m_gpu = nullptr;
    }
    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
}

App::~App() {
    shutdown();
}

} // namespace sc
