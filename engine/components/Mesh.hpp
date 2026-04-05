#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstdint>

namespace hiromi {

struct Mesh {
    SDL_GPUBuffer* vertex_buffer = nullptr;
    SDL_GPUBuffer* index_buffer  = nullptr;
    uint32_t       index_count   = 0;
    uint32_t       vertex_offset = 0;  // byte offset into vertex buffer
    uint32_t       index_offset  = 0;  // first index within index buffer

    // Index into GPUPipelineCache — which pipeline to use for this mesh.
    uint32_t pipeline_id = 0;
};

} // namespace hiromi
