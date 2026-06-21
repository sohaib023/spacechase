#include "core/App.hpp"

#include "components/Altitude.hpp"
#include "components/CollisionLayer.hpp"
#include "components/Health.hpp"
#include "components/Light.hpp"
#include "components/PrevTransform.hpp"
#include "components/ShipStats.hpp"
#include "components/Transform.hpp"
#include "components/Velocity.hpp"
#include "components/Wall.hpp"
#include "core/Log.hpp"
#include "data/LevelDef.hpp"
#include "data/LevelLoader.hpp"
#include "data/ShipDef.hpp"
#include "data/WallDef.hpp"
#include "gameplay/ShipFactory.hpp"
#include "gameplay/WallFactory.hpp"
#include "physics/Raycast.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/FloorAltitudeSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/LightingSystem.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/RenderCollectSystem.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

#include <cmath>
#include <limits>
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
    // Optional normal map (empty path → flat-normal fallback in the renderer).
    const auto loadNormal = [this](const std::string& rel) {
        return rel.empty() ? kInvalidTexture
                           : m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/" + rel);
    };
    const TextureHandle shipNormal = loadNormal(playerDef->normalSheet);

    m_floorTexture = m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/sprites/floor.png");
    m_backgroundTexture = m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/sprites/background.png");
    m_debugTexture = m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/sprites/debug_white.png");

    // Load wall definitions and the level (floor / voids / wall instances).
    m_defs.load_walls_from_dir(std::string(SC_ASSET_DIR) + "/definitions/walls");
    LevelData levelData;
    if (!load_level(std::string(SC_ASSET_DIR) + "/levels/test_m3.json", levelData)) {
        SC_LOG_ERROR("Failed to load level");
        return false;
    }
    m_level = levelData.level;
    m_spawn = levelData.playerSpawn;

    // Spawn wall instances (before the player, so the ship draws on top).
    for (const WallInstance& instance : levelData.walls) {
        const WallDef* wallDef = m_defs.find_wall(instance.def);
        if (wallDef == nullptr) {
            SC_LOG_ERROR("Unknown wall def '%s'", instance.def.c_str());
            continue;
        }
        const TextureHandle wallTexture =
            m_textures.load(m_gpu, std::string(SC_ASSET_DIR) + "/" + wallDef->sprite);
        const TextureHandle wallNormal = loadNormal(wallDef->normal);
        const glm::vec2 halfExtents =
            instance.hasHalfExtents ? instance.halfExtents : wallDef->halfExtents;
        spawn_wall(m_world.registry, wallTexture, wallNormal, instance.center, halfExtents,
                   wallDef->destructible, wallDef->health);
    }

    m_world.playerShip = spawn_ship(m_world.registry, *playerDef, shipTexture, shipNormal, m_spawn,
                                    /*facing=*/0.0f, /*playerControlled=*/true);

    // Lights from the level (directional + point), as Transform + Light entities.
    for (const LightInstance& instance : levelData.lights) {
        const Entity lightEntity = m_world.registry.create();
        m_world.registry.emplace<Transform>(lightEntity, Transform{instance.position, 0.0f});
        m_world.registry.emplace<Light>(lightEntity, instance.light);
    }

    // A draggable debug point light (ImGui-controlled) to exercise the lighting.
    m_debugLight = m_world.registry.create();
    m_world.registry.emplace<Transform>(m_debugLight, Transform{{4.0f, 2.0f}, 0.0f});
    Light debugLight;
    debugLight.type = LightType::Point;
    debugLight.color = {1.0f, 0.9f, 0.8f};
    debugLight.intensity = 3.0f;
    debugLight.height = 2.0f;
    debugLight.range = 16.0f;
    m_world.registry.emplace<Light>(m_debugLight, debugLight);

    // Camera starts on the ship and elastically follows it (translation only).
    m_camera.position = m_spawn;
    m_cameraFollow = std::make_unique<SmoothDampFollow>();

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

        // --- Elastic camera follow (translation only), targets the ship's
        //     interpolated render position. Render-time, real frame dt. ---
        if (m_world.registry.valid(m_world.playerShip)) {
            const Transform& cur = m_world.registry.get<Transform>(m_world.playerShip);
            const PrevTransform& prev = m_world.registry.get<PrevTransform>(m_world.playerShip);
            const glm::vec2 target = glm::mix(prev.position, cur.position, alpha);
            m_camera.position =
                m_cameraFollow->update(m_camera.position, target, static_cast<float>(frameTime));
        }

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
                if (ImGui::Button("damage nearest crate")) {
                    // Exercises Health + despawn (no weapons until M5). Crates are
                    // the only walls with Health.
                    Entity nearest = entt::null;
                    float best = std::numeric_limits<float>::max();
                    auto crates = reg.view<Transform, Wall, Health>();
                    for (const Entity crate : crates) {
                        const float d = glm::distance(crates.get<Transform>(crate).position,
                                                      transform.position);
                        if (d < best) {
                            best = d;
                            nearest = crate;
                        }
                    }
                    if (reg.valid(nearest)) {
                        Health& hp = reg.get<Health>(nearest);
                        hp.current -= 50.0f;
                        if (hp.current <= 0.0f) {
                            reg.destroy(nearest);
                        }
                    }
                }
                ImGui::TextDisabled("WASD / arrow keys to fly");
            }
        }
        ImGui::End();

        if (ImGui::Begin("Lighting")) {
            ImGui::ColorEdit3("ambient", &m_ambient.x);
            ImGui::SliderFloat("ambient strength", &m_ambient.w, 0.0f, 1.0f);
            ImGui::SliderFloat("shininess", &m_lightParams.x, 1.0f, 128.0f);
            ImGui::SliderFloat("specular", &m_lightParams.y, 0.0f, 2.0f);
            if (m_world.registry.valid(m_debugLight)) {
                Transform& lt = m_world.registry.get<Transform>(m_debugLight);
                Light& li = m_world.registry.get<Light>(m_debugLight);
                ImGui::SeparatorText("debug point light");
                ImGui::DragFloat2("position", &lt.position.x, 0.1f);
                ImGui::SliderFloat("height", &li.height, 0.1f, 10.0f);
                ImGui::SliderFloat("range", &li.range, 1.0f, 40.0f);
                ImGui::SliderFloat("intensity", &li.intensity, 0.0f, 8.0f);
                ImGui::ColorEdit3("color", &li.color.x);
            }
        }
        ImGui::End();

        if (ImGui::Begin("Camera")) {
            const char* models[] = {"SmoothDamp (no overshoot)", "Exponential lag"};
            if (ImGui::Combo("follow model", &m_cameraModel, models, 2)) {
                if (m_cameraModel == 0) {
                    m_cameraFollow = std::make_unique<SmoothDampFollow>();
                } else {
                    m_cameraFollow = std::make_unique<ExponentialFollow>();
                }
            }
            if (auto* sd = dynamic_cast<SmoothDampFollow*>(m_cameraFollow.get())) {
                ImGui::SliderFloat("smooth time (s)", &sd->smoothTime, 0.02f, 1.0f);
            } else if (auto* ex = dynamic_cast<ExponentialFollow*>(m_cameraFollow.get())) {
                ImGui::SliderFloat("responsiveness", &ex->responsiveness, 1.0f, 20.0f);
            }
        }
        ImGui::End();

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        // --- Queue the world (background → floor → entities) ---
        m_renderer.begin();
        {
            // One unlit, camera-locked void backdrop behind everything (the void
            // is "off into infinity"; pits are gaps in the floor that reveal it).
            int pxW = 0;
            int pxH = 0;
            SDL_GetWindowSizeInPixels(m_window, &pxW, &pxH);
            const float viewW = static_cast<float>(pxW) / (m_camera.pixelsPerMeter * m_camera.zoom);
            const float viewH = static_cast<float>(pxH) / (m_camera.pixelsPerMeter * m_camera.zoom);
            m_renderer.draw(m_backgroundTexture, m_camera.position,
                            glm::vec2(viewW * 1.2f, viewH * 1.2f), 0.0f, glm::vec2(0.0f),
                            glm::vec2(1.0f), /*unlit=*/true);
        }
        for (const AABB& floorRect : m_level.floorRects) {
            m_renderer.draw(m_floorTexture, floorRect.center, floorRect.halfExtents * 2.0f, 0.0f);
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
            m_renderer.draw(m_debugTexture, mid, glm::vec2(len, 0.06f), phi, glm::vec2(0.0f),
                            glm::vec2(1.0f), /*unlit=*/true);
            if (hit.hit) {
                m_renderer.draw(m_debugTexture, hit.point, glm::vec2(0.4f, 0.4f), 0.0f,
                                glm::vec2(0.0f), glm::vec2(1.0f), /*unlit=*/true);
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

            const LightsUBO lights = collect_lights(m_world.registry, m_ambient, m_lightParams);

            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
            m_renderer.render(cmd, pass, viewProj, m_textures, lights);
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
