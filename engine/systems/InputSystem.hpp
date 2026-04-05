#pragma once

#include "../ecs/System.hpp"

namespace hiromi {

// Reads SDL events each fixed tick and updates the InputState World resource.
// Sets running to false when SDL_EVENT_QUIT (or Escape) is received.
class InputSystem final : public ISystem {
public:
    // running is the engine's main-loop flag; InputSystem sets it to false to quit.
    explicit InputSystem(bool& running) : running_(running) {}

    void update(World& world, float dt) override;
    [[nodiscard]] std::string_view name() const noexcept override { return "InputSystem"; }

private:
    bool& running_;
};

} // namespace hiromi
