#include "core/App.hpp"

#include "core/Log.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

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

    // Request the shader formats SDL_shadercross targets (Vulkan/D3D12/Metal).
    // M0 uses no shaders of its own, but ImGui's SDL_GPU backend needs a device.
    m_gpu = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
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

    SC_LOG_INFO("Space Chase initialized.");
    return true;
}

void App::run() {
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

        // Build the UI for this frame.
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();

        ImDrawData* drawData = ImGui::GetDrawData();

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
            // The SDL_GPU backend uploads vertex/index data via a copy pass, which
            // must run before any render pass begins (brief notes this constraint).
            ImGui_ImplSDLGPU3_PrepareDrawData(drawData, cmd);

            SDL_GPUColorTargetInfo target{};
            target.texture = swapchain;
            target.clear_color = kClearColor;
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
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
