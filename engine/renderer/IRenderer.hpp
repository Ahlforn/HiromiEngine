#pragma once

#include <SDL3/SDL.h>
#include <string_view>

namespace hiromi {

// Abstract renderer interface. Called by Engine each frame.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Create the renderer's device/context and bind it to the window.
    virtual bool initialize(SDL_Window* window) = 0;

    // Destroy all renderer resources.
    virtual void shutdown() = 0;

    // Acquire a command buffer / begin frame recording.
    virtual void begin_frame() = 0;

    // Flush all queued draw commands, submit, and present.
    virtual void end_frame() = 0;

    // Recreate size-dependent resources (depth texture, swap chain, etc.).
    virtual void on_resize(int new_w, int new_h) = 0;

    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
};

} // namespace hiromi
