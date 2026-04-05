#pragma once

namespace hiromi {

struct AnimationState {
    int   current_frame = 0;
    int   frame_count   = 1;
    float fps           = 12.0f;

    // Accumulated time since the last frame advance.
    float elapsed = 0.0f;

    bool looping = true;
    bool playing = true;

    // When true, the Sprite's src_x is updated to frame * frame_pixel_width.
    // Frame pixel width must match the Sprite's src_w.
    float frame_pixel_width  = 0.0f;
    float frame_pixel_height = 0.0f;
};

} // namespace hiromi
