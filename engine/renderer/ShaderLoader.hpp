#pragma once

#include <SDL3/SDL_gpu.h>
#include <string>
#include <string_view>
#include <cstdint>

namespace hiromi {

// Loads SPIR-V .spv blobs from disk and creates SDL_GPUShader objects.
// Shaders are owned by the caller and must be released with SDL_ReleaseGPUShader.
class ShaderLoader {
public:
    explicit ShaderLoader(SDL_GPUDevice* device);

    // Loads a SPIR-V shader from a file path.
    // stage: SDL_GPU_SHADERSTAGE_VERTEX or SDL_GPU_SHADERSTAGE_FRAGMENT
    // entry_point: typically "main"
    // num_samplers / num_uniform_buffers: must match the shader's layout.
    [[nodiscard]] SDL_GPUShader* load(
        const std::string& path,
        SDL_GPUShaderStage stage,
        const char*        entry_point        = "main",
        uint32_t           num_samplers       = 0,
        uint32_t           num_uniform_buffers = 0,
        uint32_t           num_storage_buffers = 0,
        uint32_t           num_storage_textures = 0);

private:
    SDL_GPUDevice* device_ = nullptr;
};

} // namespace hiromi
