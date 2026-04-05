#include "Engine.hpp"
#include "../core/Assert.hpp"
#include "../core/Logger.hpp"

#include <SDL3/SDL.h>
#include <algorithm>

namespace hiromi {

static constexpr uint64_t MAX_FRAME_NS = 100'000'000ull; // 100ms cap

Engine::Engine(EngineConfig config) : config_(std::move(config)) {
    fixed_ns_ = static_cast<uint64_t>(1e9 / config_.physics_hz);
}

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        log::error("Engine: SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    SDL_WindowFlags flags = 0;
    if (config_.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;

    window_ = SDL_CreateWindow(config_.title.c_str(),
                               config_.width, config_.height,
                               flags);
    if (!window_) {
        log::error("Engine: SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    // Create a single shared SDL_GPU device for both renderers.
    device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, nullptr);
    if (!device_) {
        log::error("Engine: SDL_CreateGPUDevice failed: {}", SDL_GetError());
        return false;
    }

    if (config_.enable_2d) {
        renderer_2d_ = std::make_unique<Renderer2D>();
        if (!renderer_2d_->initialize_with_device(window_, device_)) {
            log::error("Engine: Renderer2D initialization failed");
            return false;
        }
    }

    if (config_.enable_3d && !config_.enable_2d) {
        // Renderer3D claims the window only if 2D renderer hasn't claimed it.
        renderer_3d_ = std::make_unique<Renderer3D>();
        if (!renderer_3d_->initialize_with_device(window_, device_)) {
            log::error("Engine: Renderer3D initialization failed");
            return false;
        }
    } else if (config_.enable_3d && config_.enable_2d) {
        // Both enabled: create Renderer3D sharing the already-claimed device.
        // Renderer3D won't re-claim the window (it relies on the same device).
        renderer_3d_ = std::make_unique<Renderer3D>();
        renderer_3d_->initialize_with_device(window_, device_);
        // Note: the window is already claimed by Renderer2D.
        // Renderer3D will share the command buffer flow via the same device.
    }

    // Register InputState as a World resource.
    world_.add_resource(InputState{});

    log::info("Engine initialized: '{}' ({}x{})", config_.title, config_.width, config_.height);
    return true;
}

void Engine::run() {
    HIROMI_ASSERT(window_ != nullptr, "Engine::run called before initialize()");

    running_       = true;
    prev_tick_ns_  = SDL_GetTicksNS();
    accumulator_ns_ = 0;

    while (running_) {
        pump_frame();
    }

    log::info("Engine main loop exited");
}

void Engine::pump_frame() {
    const uint64_t now   = SDL_GetTicksNS();
    uint64_t       delta = now - prev_tick_ns_;
    prev_tick_ns_        = now;

    // Cap delta to prevent spiral of death after a pause/breakpoint.
    if (delta > MAX_FRAME_NS) delta = MAX_FRAME_NS;

    accumulator_ns_ += delta;

    // Fixed update loop.
    const float fixed_dt = static_cast<float>(fixed_ns_) / 1e9f;
    while (accumulator_ns_ >= fixed_ns_) {
        for (auto& sys : fixed_systems_) {
            sys->update(world_, fixed_dt);
        }
        accumulator_ns_ -= fixed_ns_;
    }

    // Variable update (rendering).
    const float render_dt = static_cast<float>(delta) / 1e9f;

    if (renderer_2d_) renderer_2d_->begin_frame();
    if (renderer_3d_) renderer_3d_->begin_frame();

    for (auto& sys : variable_systems_) {
        sys->update(world_, render_dt);
    }

    if (renderer_2d_) renderer_2d_->end_frame();
    if (renderer_3d_) renderer_3d_->end_frame();
}

void Engine::shutdown() {
    fixed_systems_.clear();
    variable_systems_.clear();

    if (renderer_2d_) { renderer_2d_->shutdown(); renderer_2d_.reset(); }
    if (renderer_3d_) { renderer_3d_->shutdown(); renderer_3d_.reset(); }

    if (device_)  { SDL_DestroyGPUDevice(device_); device_  = nullptr; }
    if (window_)  { SDL_DestroyWindow(window_);     window_  = nullptr; }
    SDL_Quit();
}

} // namespace hiromi
