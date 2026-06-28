#pragma once

// The application shell: owns the SDL window and GPU device, runs the main
// loop, and tears everything down in order. M0 only clears the screen and
// renders the ImGui demo window; the fixed-timestep simulation/render split
// (brief §3.4) lands in M1 when there is something to simulate.

#include "data/DefinitionRegistry.hpp"
#include "ecs/World.hpp"
#include "gameplay/FallConfig.hpp"
#include "gameplay/Level.hpp"
#include "gameplay/Spawning.hpp"
#include "render/Camera.hpp"
#include "render/CameraFollow.hpp"
#include "render/Renderer.hpp"
#include "resources/TextureRegistry.hpp"

#include <glm/glm.hpp>

#include <memory>

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
    TextureRegistry m_textures;
    Camera2D m_camera;
    std::unique_ptr<CameraFollow> m_cameraFollow; // swappable elastic follow
    int m_cameraModel = 0;                         // 0 = SmoothDamp, 1 = Exponential

    World m_world;             // entity registry + named handles (owns the sim)
    DefinitionRegistry m_defs; // loaded content definitions (source of truth)
    Level m_level;             // floor + void rects (continuous meters)
    FallConfig m_fallConfig;   // pseudo-3D falling tunables
    Deferred m_deferred;       // per-step spawn/despawn queues (flushed each step)
    glm::vec2 m_spawn{0.0f, 0.0f};

    TextureHandle m_debugTexture = kInvalidTexture; // debug-draw marker/beam
    TextureHandle m_floorTexture = kInvalidTexture;
    TextureHandle m_backgroundTexture = kInvalidTexture; // unlit void backdrop
    bool m_showDebugRay = true;

    glm::vec4 m_ambient{0.12f, 0.12f, 0.18f, 1.0f};   // rgb + strength
    glm::vec4 m_lightParams{32.0f, 0.5f, 0.0f, 0.0f}; // shininess, specular strength
    Entity m_debugLight = entt::null;                 // ImGui-draggable light
};

} // namespace sc
