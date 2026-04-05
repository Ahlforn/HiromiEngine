#include "InputSystem.hpp"
#include "../ecs/World.hpp"
#include "../input/InputState.hpp"

#include <SDL3/SDL.h>

namespace hiromi {

void InputSystem::update(World& world, float /*dt*/) {
    if (!world.has_resource<InputState>()) return;

    InputState& state = world.get_resource<InputState>();
    state.clear_frame_state();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                running_ = false;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode < SDL_SCANCODE_COUNT) {
                    state.keys_held[event.key.scancode]    = true;
                    state.keys_pressed[event.key.scancode] = true;
                }
                break;

            case SDL_EVENT_KEY_UP:
                if (event.key.scancode < SDL_SCANCODE_COUNT) {
                    state.keys_held[event.key.scancode]     = false;
                    state.keys_released[event.key.scancode] = true;
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                state.mouse_pos   = {event.motion.x, event.motion.y};
                state.mouse_delta = {event.motion.xrel, event.motion.yrel};
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                int btn = event.button.button - 1; // SDL buttons are 1-indexed
                if (btn >= 0 && btn < 5) {
                    state.mouse_held[btn]    = true;
                    state.mouse_pressed[btn] = true;
                }
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP: {
                int btn = event.button.button - 1;
                if (btn >= 0 && btn < 5) {
                    state.mouse_held[btn]     = false;
                    state.mouse_released[btn] = true;
                }
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
                state.mouse_scroll = {event.wheel.x, event.wheel.y};
                break;

            default:
                break;
        }
    }
}

} // namespace hiromi
