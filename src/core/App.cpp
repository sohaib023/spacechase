#include "core/App.hpp"

#include "core/Log.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

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

    const std::string shipPath = std::string(SC_ASSET_DIR) + "/sprites/ship_sheet.png";
    if (!load_texture(m_gpu, shipPath.c_str(), m_shipTexture)) {
        return false;
    }

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

        // --- Fixed-step simulation ---
        while (accumulator >= kFixedDt) {
            m_shipPrev = m_ship;
            ship_step(m_ship, m_shipConfig, actions, kFixedDt);
            accumulator -= kFixedDt;
        }
        const float alpha = static_cast<float>(accumulator / kFixedDt);
        const ShipState rendered = ship_lerp(m_shipPrev, m_ship, alpha);

        // --- UI ---
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("Ship")) {
            ImGui::Text("pos    (%.0f, %.0f)", m_ship.position.x, m_ship.position.y);
            ImGui::Text("vel    (%.0f, %.0f)", m_ship.velocity.x, m_ship.velocity.y);
            ImGui::Text("speed  %.0f", glm::length(m_ship.velocity));
            ImGui::Text("facing %.2f rad", m_ship.facing);
            ImGui::Separator();
            ImGui::SliderFloat("acceleration", &m_shipConfig.acceleration, 100.0f, 2500.0f);
            ImGui::SliderFloat("max speed", &m_shipConfig.maxSpeed, 100.0f, 1500.0f);
            ImGui::SliderFloat("turn speed", &m_shipConfig.turnSpeed, 1.0f, 8.0f);
            ImGui::SliderFloat("drag", &m_shipConfig.drag, 0.0f, 5.0f);
            ImGui::TextDisabled("WASD / arrow keys to fly");
        }
        ImGui::End();
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        // --- Queue the world into the sprite batch ---
        m_renderer.begin();
        {
            // Tilt frame from turn input: 0 = bank-left, 1 = neutral, 2 = bank-right.
            int frame = 1;
            if (actions.turn > 0.2f) {
                frame = 0;
            } else if (actions.turn < -0.2f) {
                frame = 2;
            }
            constexpr float kFrameW = 1.0f / 3.0f;
            const glm::vec2 uvMin{static_cast<float>(frame) * kFrameW, 0.0f};
            const glm::vec2 uvMax{static_cast<float>(frame + 1) * kFrameW, 1.0f};
            // One sheet frame is 64x64 (sheet is 192x64).
            m_renderer.draw(m_shipTexture, rendered.position, glm::vec2(64.0f, 64.0f),
                            rendered.facing, uvMin, uvMax);
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
            m_renderer.render(cmd, pass, viewProj);
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
        destroy_texture(m_gpu, m_shipTexture);
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
