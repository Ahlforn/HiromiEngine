#pragma once

#include <SDL3/SDL.h>
#include <glm/vec2.hpp>
#include <array>

namespace hiromi {

// Stateless per-frame snapshot of keyboard and mouse input.
// Updated exclusively by InputSystem at the start of each fixed tick.
// All other systems read this via world.get_resource<InputState>().
struct InputState {
    // --- Keyboard ---
    // keys_held:     true while the key is physically down
    // keys_pressed:  true for exactly one tick when the key goes down
    // keys_released: true for exactly one tick when the key goes up
    std::array<bool, SDL_SCANCODE_COUNT> keys_held     = {};
    std::array<bool, SDL_SCANCODE_COUNT> keys_pressed  = {};
    std::array<bool, SDL_SCANCODE_COUNT> keys_released = {};

    // --- Mouse ---
    glm::vec2 mouse_pos    = {0.0f, 0.0f};  // window-space pixels
    glm::vec2 mouse_delta  = {0.0f, 0.0f};  // relative motion this tick
    glm::vec2 mouse_scroll = {0.0f, 0.0f};  // wheel delta this tick

    // Buttons: index 0=left, 1=middle, 2=right, 3=x1, 4=x2
    std::array<bool, 5> mouse_held     = {};
    std::array<bool, 5> mouse_pressed  = {};
    std::array<bool, 5> mouse_released = {};

    // --- Helpers ---

    [[nodiscard]] bool key(SDL_Scancode sc) const noexcept {
        return static_cast<int>(sc) < SDL_SCANCODE_COUNT && keys_held[sc];
    }
    [[nodiscard]] bool key_down(SDL_Scancode sc) const noexcept {
        return static_cast<int>(sc) < SDL_SCANCODE_COUNT && keys_pressed[sc];
    }
    [[nodiscard]] bool key_up(SDL_Scancode sc) const noexcept {
        return static_cast<int>(sc) < SDL_SCANCODE_COUNT && keys_released[sc];
    }

    // btn: 0=left, 1=middle, 2=right
    [[nodiscard]] bool mouse_button(int btn) const noexcept {
        return btn >= 0 && btn < 5 && mouse_held[btn];
    }
    [[nodiscard]] bool mouse_button_down(int btn) const noexcept {
        return btn >= 0 && btn < 5 && mouse_pressed[btn];
    }
    [[nodiscard]] bool mouse_button_up(int btn) const noexcept {
        return btn >= 0 && btn < 5 && mouse_released[btn];
    }

    // Called by InputSystem at the start of each tick to zero per-frame arrays.
    void clear_frame_state() noexcept {
        keys_pressed.fill(false);
        keys_released.fill(false);
        mouse_pressed.fill(false);
        mouse_released.fill(false);
        mouse_delta  = {0.0f, 0.0f};
        mouse_scroll = {0.0f, 0.0f};
    }
};

} // namespace hiromi
