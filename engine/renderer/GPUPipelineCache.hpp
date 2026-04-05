#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace hiromi {

// Identifies a unique graphics pipeline configuration.
struct PipelineKey {
    uint32_t vert_shader_id  = 0;
    uint32_t frag_shader_id  = 0;
    SDL_GPUTextureFormat color_format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    SDL_GPUTextureFormat depth_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    SDL_GPUPrimitiveType prim_type    = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    bool                 depth_test   = true;
    bool                 depth_write  = true;

    bool operator==(const PipelineKey&) const noexcept = default;

    struct Hash {
        std::size_t operator()(const PipelineKey& k) const noexcept {
            std::size_t h = 14695981039346656037ull;
            auto mix = [&](uint64_t v) {
                h ^= v;
                h *= 1099511628211ull;
            };
            mix(k.vert_shader_id);
            mix(k.frag_shader_id);
            mix(static_cast<uint64_t>(k.color_format));
            mix(static_cast<uint64_t>(k.depth_format));
            mix(static_cast<uint64_t>(k.prim_type));
            mix(k.depth_test ? 1u : 0u);
            mix(k.depth_write ? 1u : 0u);
            return h;
        }
    };
};

class GPUPipelineCache {
public:
    explicit GPUPipelineCache(SDL_GPUDevice* device);
    ~GPUPipelineCache();

    // Returns an existing pipeline matching the key, or creates and caches one.
    // create_info is only used on cache miss.
    [[nodiscard]] SDL_GPUGraphicsPipeline* get_or_create(
        const PipelineKey&                        key,
        const SDL_GPUGraphicsPipelineCreateInfo&  create_info);

    // Destroys all cached pipelines.
    void clear();

private:
    SDL_GPUDevice* device_ = nullptr;
    std::unordered_map<PipelineKey, SDL_GPUGraphicsPipeline*, PipelineKey::Hash> cache_;
};

} // namespace hiromi
