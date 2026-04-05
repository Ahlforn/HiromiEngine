#pragma once

#include "../ecs/World.hpp"
#include "../ecs/System.hpp"
#include "../renderer/Renderer2D.hpp"
#include "../renderer/Renderer3D.hpp"
#include "../input/InputState.hpp"

#include <SDL3/SDL.h>
#include <memory>
#include <vector>
#include <string>

namespace hiromi {

enum class SystemSchedule {
    FIXED,    // Called at a fixed timestep (default: 60 Hz)
    VARIABLE, // Called once per rendered frame at variable dt
};

struct EngineConfig {
    std::string title      = "Hiromi Engine";
    int         width      = 1280;
    int         height     = 720;
    bool        vsync      = true;
    bool        fullscreen = false;
    float       physics_hz = 60.0f;

    // Which rendering mode to use. Using both simultaneously is not supported
    // (SDL_GPU and SDL_Renderer share the same window handle exclusively).
    bool enable_2d = true;
    bool enable_3d = false;
};

class Engine {
public:
    explicit Engine(EngineConfig config = {});
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    // Initializes SDL, creates window, initializes renderer(s), and registers
    // the InputState World resource. Returns false on failure.
    bool initialize();

    // Runs the main loop until quit. Blocks until the engine stops.
    void run();

    // Gracefully stops the main loop (called by InputSystem on SDL_EVENT_QUIT).
    void quit() noexcept { running_ = false; }

    // Add a system to the execution pipeline.
    // Systems run in insertion order within their schedule group.
    template<typename T, typename... Args>
    T& add_system(SystemSchedule schedule, Args&&... args) {
        auto sys = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref   = *sys;
        if (schedule == SystemSchedule::FIXED) {
            fixed_systems_.push_back(std::move(sys));
        } else {
            variable_systems_.push_back(std::move(sys));
        }
        return ref;
    }

    [[nodiscard]] World&       world()        noexcept { return world_; }
    [[nodiscard]] Renderer2D*  renderer_2d() noexcept { return renderer_2d_.get(); }
    [[nodiscard]] Renderer3D*  renderer_3d() noexcept { return renderer_3d_.get(); }
    [[nodiscard]] SDL_Window*  window()       noexcept { return window_; }

    // Returns a reference to the main-loop running flag.
    // Pass to InputSystem so it can stop the engine on SDL_EVENT_QUIT.
    [[nodiscard]] bool& running_flag() noexcept { return running_; }

private:
    void shutdown();
    void pump_frame();

    EngineConfig config_;

    SDL_Window*               window_   = nullptr;
    SDL_GPUDevice*            device_   = nullptr; // shared by both renderers

    World                             world_;
    std::unique_ptr<Renderer2D>       renderer_2d_;
    std::unique_ptr<Renderer3D>       renderer_3d_;

    std::vector<std::unique_ptr<ISystem>> fixed_systems_;
    std::vector<std::unique_ptr<ISystem>> variable_systems_;

    bool running_ = false;

    // Timing state
    uint64_t prev_tick_ns_    = 0;
    uint64_t accumulator_ns_  = 0;
    uint64_t fixed_ns_        = 0; // derived from config_.physics_hz
};

} // namespace hiromi
