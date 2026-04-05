#include "GPUPipelineCache.hpp"
#include "../core/Assert.hpp"
#include "../core/Logger.hpp"

namespace hiromi {

GPUPipelineCache::GPUPipelineCache(SDL_GPUDevice* device) : device_(device) {
    HIROMI_ASSERT(device_ != nullptr, "GPUPipelineCache: null device");
}

GPUPipelineCache::~GPUPipelineCache() {
    clear();
}

SDL_GPUGraphicsPipeline* GPUPipelineCache::get_or_create(
    const PipelineKey&                       key,
    const SDL_GPUGraphicsPipelineCreateInfo& create_info)
{
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &create_info);
    if (!pipeline) {
        log::error("GPUPipelineCache: SDL_CreateGPUGraphicsPipeline failed: {}",
                   SDL_GetError());
        return nullptr;
    }

    cache_[key] = pipeline;
    log::debug("GPUPipelineCache: created pipeline (cache size = {})", cache_.size());
    return pipeline;
}

void GPUPipelineCache::clear() {
    for (auto& [key, pipeline] : cache_) {
        SDL_ReleaseGPUGraphicsPipeline(device_, pipeline);
    }
    cache_.clear();
}

} // namespace hiromi
