#include "Renderer3D.hpp"
#include "../core/Assert.hpp"
#include "../core/Logger.hpp"

#include <SDL3/SDL.h>
#include <glm/gtc/type_ptr.hpp>

namespace hiromi {

Renderer3D::~Renderer3D() {
    shutdown();
}

bool Renderer3D::initialize(SDL_Window* window) {
    SDL_GPUDevice* dev = SDL_CreateGPUDevice(preferred_shader_format(), false, nullptr);
    if (!dev) {
        log::error("Renderer3D: SDL_CreateGPUDevice failed: {}", SDL_GetError());
        return false;
    }
    owns_device_ = true;
    return initialize_with_device(window, dev);
}

bool Renderer3D::initialize_with_device(SDL_Window* window, SDL_GPUDevice* device) {
    window_  = window;
    device_  = device;

    if (!SDL_ClaimWindowForGPUDevice(device_, window_)) {
        log::error("Renderer3D: SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError());
        return false;
    }

    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    fb_w_ = w; fb_h_ = h;

    pipeline_cache_ = std::make_unique<GPUPipelineCache>(device_);
    recreate_depth_texture(fb_w_, fb_h_);

    log::info("Renderer3D initialized ({}x{})", fb_w_, fb_h_);
    return true;
}

void Renderer3D::shutdown() {
    if (!device_) return;

    SDL_WaitForGPUIdle(device_);

    pipeline_cache_.reset();
    if (depth_tex_) { SDL_ReleaseGPUTexture(device_, depth_tex_); depth_tex_ = nullptr; }

    if (owns_device_) SDL_DestroyGPUDevice(device_);
    device_ = nullptr;
    log::info("Renderer3D shutdown");
}

void Renderer3D::begin_frame() {
    HIROMI_ASSERT(device_ != nullptr, "Renderer3D not initialized");
    cmd_buf_ = SDL_AcquireGPUCommandBuffer(device_);
    HIROMI_ASSERT(cmd_buf_ != nullptr, "Renderer3D: SDL_AcquireGPUCommandBuffer failed");
    draw_queue_.clear();
}

void Renderer3D::end_frame() {
    HIROMI_ASSERT(cmd_buf_ != nullptr, "Renderer3D::end_frame without begin_frame");

    SDL_GPUTexture* swap_tex = nullptr;
    uint32_t sw = 0, sh = 0;
    SDL_AcquireGPUSwapchainTexture(cmd_buf_, window_, &swap_tex, &sw, &sh);

    if (swap_tex) {
        flush_draw_commands(cmd_buf_, swap_tex);
    }

    SDL_SubmitGPUCommandBuffer(cmd_buf_);
    cmd_buf_ = nullptr;
}

void Renderer3D::on_resize(int new_w, int new_h) {
    if (new_w == fb_w_ && new_h == fb_h_) return;
    fb_w_ = new_w; fb_h_ = new_h;
    if (device_) {
        SDL_WaitForGPUIdle(device_);
        recreate_depth_texture(fb_w_, fb_h_);
    }
}

std::string_view Renderer3D::backend_name() const noexcept {
    return "SDL_GPU/3D";
}

void Renderer3D::submit_draw(const DrawCommand3D& cmd) {
    draw_queue_.push_back(cmd);
}

void Renderer3D::set_view_projection(const glm::mat4& view, const glm::mat4& proj) {
    view_ = view;
    proj_ = proj;
}

SDL_GPUBuffer* Renderer3D::create_vertex_buffer(const void* data, uint32_t bytes) {
    SDL_GPUBufferCreateInfo bci{};
    bci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size  = bytes;
    SDL_GPUBuffer* buf = SDL_CreateGPUBuffer(device_, &bci);
    if (!buf) {
        log::error("Renderer3D: create_vertex_buffer failed: {}", SDL_GetError());
        return nullptr;
    }

    // Upload via transfer buffer.
    SDL_GPUTransferBufferCreateInfo tbci{};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size  = bytes;
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbci);
    auto* map = SDL_MapGPUTransferBuffer(device_, tb, false);
    memcpy(map, data, bytes);
    SDL_UnmapGPUTransferBuffer(device_, tb);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device_);
    SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src{tb, 0};
    SDL_GPUBufferRegion dst{buf, 0, bytes};
    SDL_UploadToGPUBuffer(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device_, tb);
    SDL_WaitForGPUIdle(device_);

    return buf;
}

SDL_GPUBuffer* Renderer3D::create_index_buffer(const void* data, uint32_t bytes) {
    SDL_GPUBufferCreateInfo bci{};
    bci.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    bci.size  = bytes;
    SDL_GPUBuffer* buf = SDL_CreateGPUBuffer(device_, &bci);
    if (!buf) {
        log::error("Renderer3D: create_index_buffer failed: {}", SDL_GetError());
        return nullptr;
    }

    SDL_GPUTransferBufferCreateInfo tbci{};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size  = bytes;
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbci);
    auto* map = SDL_MapGPUTransferBuffer(device_, tb, false);
    memcpy(map, data, bytes);
    SDL_UnmapGPUTransferBuffer(device_, tb);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device_);
    SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src{tb, 0};
    SDL_GPUBufferRegion dst{buf, 0, bytes};
    SDL_UploadToGPUBuffer(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device_, tb);
    SDL_WaitForGPUIdle(device_);

    return buf;
}

void Renderer3D::release_buffer(SDL_GPUBuffer* buf) {
    if (buf && device_) SDL_ReleaseGPUBuffer(device_, buf);
}

// ---- Private ----------------------------------------------------------------

void Renderer3D::recreate_depth_texture(int w, int h) {
    if (depth_tex_) { SDL_ReleaseGPUTexture(device_, depth_tex_); depth_tex_ = nullptr; }

    SDL_GPUTextureCreateInfo tci{};
    tci.type             = SDL_GPU_TEXTURETYPE_2D;
    tci.format           = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    tci.usage            = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    tci.width            = static_cast<uint32_t>(w);
    tci.height           = static_cast<uint32_t>(h);
    tci.layer_count_or_depth = 1;
    tci.num_levels       = 1;

    depth_tex_ = SDL_CreateGPUTexture(device_, &tci);
    HIROMI_ASSERT(depth_tex_, "Renderer3D: failed to create depth texture");
}

void Renderer3D::flush_draw_commands(SDL_GPUCommandBuffer* cmd_buf, SDL_GPUTexture* swap_tex) {
    SDL_GPUColorTargetInfo cti{};
    cti.texture     = swap_tex;
    cti.load_op     = SDL_GPU_LOADOP_CLEAR;
    cti.store_op    = SDL_GPU_STOREOP_STORE;
    cti.clear_color = {0.05f, 0.05f, 0.1f, 1.0f};

    SDL_GPUDepthStencilTargetInfo dsti{};
    dsti.texture           = depth_tex_;
    dsti.load_op           = SDL_GPU_LOADOP_CLEAR;
    dsti.store_op          = SDL_GPU_STOREOP_DONT_CARE;
    dsti.clear_depth       = 1.0f;
    dsti.cycle             = true;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd_buf, &cti, 1, &dsti);

    SDL_GPUGraphicsPipeline* last_pipeline = nullptr;

    for (const auto& draw : draw_queue_) {
        // Pipeline lookup by pipeline_id is done via the cache.
        // For now, skip draws with no pipeline registered.
        // Systems are responsible for ensuring pipelines exist before submitting draws.

        MeshPushConstants pc;
        pc.mvp = proj_ * view_ * draw.model_matrix;
        SDL_PushGPUVertexUniformData(cmd_buf, 0, &pc, sizeof(pc));

        SDL_GPUBufferBinding vb{draw.vertex_buffer, static_cast<Uint32>(draw.vertex_offset)};
        SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);

        SDL_GPUBufferBinding ib{draw.index_buffer, 0};
        SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_DrawGPUIndexedPrimitives(pass, draw.index_count, 1,
                                     draw.first_index, draw.vertex_offset, 0);
    }

    SDL_EndGPURenderPass(pass);
}

} // namespace hiromi
