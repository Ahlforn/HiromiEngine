#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstdint>

namespace hiromi {

struct Sprite {
    // Texture created via SDL_GPU. May be null for untextured quads.
    SDL_GPUTexture* texture  = nullptr;

    // Source rectangle within the texture (pixels).
    // {0,0,0,0} means use the whole texture.
    float src_x = 0.0f, src_y = 0.0f;
    float src_w = 0.0f, src_h = 0.0f;

    // Width/height in world units for the rendered quad.
    float width  = 1.0f;
    float height = 1.0f;

    // Higher z_order renders on top.
    int z_order = 0;

    // Tint colour [0,1].
    float color_r = 1.0f, color_g = 1.0f, color_b = 1.0f, color_a = 1.0f;

    // Flip the sprite horizontally or vertically.
    bool flip_x = false;
    bool flip_y = false;
};

} // namespace hiromi
