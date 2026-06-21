#pragma once

// The application shell: owns the SDL window and GPU device, runs the main
// loop, and tears everything down in order. M0 only clears the screen and
// renders the ImGui demo window; the fixed-timestep simulation/render split
// (brief §3.4) lands in M1 when there is something to simulate.

#include "gameplay/Ship.hpp"
#include "render/Camera.hpp"
#include "render/Renderer.hpp"
#include "resources/Texture.hpp"

struct SDL_Window;
struct SDL_GPUDevice;

namespace sc {

class App {
public:
    App() = default;
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Initialize SDL, window, GPU device, and ImGui. Returns false on failure
    // (the error is logged); the caller should exit non-zero.
    bool init();

    // Run the main loop until the window is closed.
    void run();

    // Release everything in reverse order of init. Safe to call multiple times.
    void shutdown();

private:
    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_gpu = nullptr;
    bool m_imguiInitialized = false;

    Renderer m_renderer;
    Texture m_shipTexture;
    Camera2D m_camera;

    ShipState m_ship;
    ShipState m_shipPrev; // previous fixed-step state, for render interpolation
    ShipConfig m_shipConfig;
};

} // namespace sc
