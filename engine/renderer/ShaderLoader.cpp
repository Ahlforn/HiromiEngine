#include "ShaderLoader.hpp"
#include "../core/Assert.hpp"
#include "../core/Logger.hpp"

#include <SDL3/SDL.h>
#include <vector>
#include <fstream>

namespace hiromi {

ShaderLoader::ShaderLoader(SDL_GPUDevice* device) : device_(device) {
    HIROMI_ASSERT(device_ != nullptr, "ShaderLoader: null device");
}

SDL_GPUShader* ShaderLoader::load(
    const std::string& path,
    SDL_GPUShaderStage stage,
    const char*        entry_point,
    uint32_t           num_samplers,
    uint32_t           num_uniform_buffers,
    uint32_t           num_storage_buffers,
    uint32_t           num_storage_textures)
{
    // Read .spv file into a byte vector.
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        log::error("ShaderLoader: cannot open '{}'", path);
        return nullptr;
    }

    const auto size = static_cast<std::size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> code(size);
    if (!file.read(reinterpret_cast<char*>(code.data()),
                   static_cast<std::streamsize>(size))) {
        log::error("ShaderLoader: read error for '{}'", path);
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};
    info.code             = code.data();
    info.code_size        = code.size();
    info.entrypoint       = entry_point;
    info.format           = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage            = stage;
    info.num_samplers     = num_samplers;
    info.num_uniform_buffers  = num_uniform_buffers;
    info.num_storage_buffers  = num_storage_buffers;
    info.num_storage_textures = num_storage_textures;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device_, &info);
    if (!shader) {
        log::error("ShaderLoader: SDL_CreateGPUShader failed for '{}': {}",
                   path, SDL_GetError());
        return nullptr;
    }

    log::debug("ShaderLoader: loaded shader '{}'", path);
    return shader;
}

} // namespace hiromi
