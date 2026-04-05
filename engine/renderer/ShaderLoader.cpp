#include "ShaderLoader.hpp"
#include "../core/Assert.hpp"
#include "../core/Logger.hpp"

#include <SDL3/SDL.h>
#include <vector>
#include <fstream>

namespace hiromi {

SDL_GPUShaderFormat preferred_shader_format() {
#ifdef __APPLE__
    return SDL_GPU_SHADERFORMAT_MSL;
#else
    return SDL_GPU_SHADERFORMAT_SPIRV;
#endif
}

const char* shader_extension(SDL_GPUShaderFormat format) {
    return (format == SDL_GPU_SHADERFORMAT_MSL) ? ".msl" : ".spv";
}

std::string shader_path(const std::string& name, SDL_GPUShaderFormat format) {
    return "assets/shaders/compiled/" + name + shader_extension(format);
}

ShaderLoader::ShaderLoader(SDL_GPUDevice* device) : device_(device) {
    HIROMI_ASSERT(device_ != nullptr, "ShaderLoader: null device");
}

SDL_GPUShader* ShaderLoader::load(
    const std::string& path,
    SDL_GPUShaderStage stage,
    SDL_GPUShaderFormat format,
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

    // spirv-cross renames "main" → "main0" in MSL output.
    const char* effective_entry = entry_point;
    if (format == SDL_GPU_SHADERFORMAT_MSL &&
        std::string_view(entry_point) == "main") {
        effective_entry = "main0";
    }

    SDL_GPUShaderCreateInfo info{};
    info.code             = code.data();
    info.code_size        = code.size();
    info.entrypoint       = effective_entry;
    info.format           = format;
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
