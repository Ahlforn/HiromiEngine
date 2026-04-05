#pragma once

#include <cstdint>

namespace hiromi {

// Data component for audio playback.
// Interpreted by an AudioSystem (future work; not included in v1 systems).
struct AudioSource {
    uint32_t sound_clip_handle = 0;  // index into an asset/sound registry
    float    volume            = 1.0f;
    float    pitch             = 1.0f;
    bool     looping           = false;
    bool     playing           = false;
    bool     spatial           = false;  // use entity position for 3D audio
};

} // namespace hiromi
