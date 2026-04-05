#include "Renderer2D.hpp"
#include "../core/Assert.hpp"
#include "../core/Logger.hpp"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace hiromi {

Renderer2D::~Renderer2D() {
    shutdown();
}

bool Renderer2D::initialize(SDL_Window* window) {
    // Create our own device when called standalone (not sharing with Renderer3D).
    SDL_GPUDevice* dev = SDL_CreateGPUDevice(preferred_shader_format(), false, nullptr);
    if (!dev) {
        log::error("Renderer2D: SDL_CreateGPUDevice failed: {}", SDL_GetError());
        return false;
    }
    owns_device_ = true;
    return initialize_with_device(window, dev);
}

bool Renderer2D::initialize_with_device(SDL_Window* window, SDL_GPUDevice* device) {
    window_  = window;
    device_  = device;

    if (!SDL_ClaimWindowForGPUDevice(device_, window_)) {
        log::error("Renderer2D: SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError());
        return false;
    }

    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    fb_w_ = w; fb_h_ = h;

    pipeline_cache_ = std::make_unique<GPUPipelineCache>(device_);

    recreate_depth_texture(fb_w_, fb_h_);
    build_pipeline();

    // Create a 1x1 white texture for untextured sprites.
    {
        SDL_GPUTextureCreateInfo tci{};
        tci.type   = SDL_GPU_TEXTURETYPE_2D;
        tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        tci.usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        tci.width  = 1;
        tci.height = 1;
        tci.layer_count_or_depth = 1;
        tci.num_levels = 1;
        white_tex_ = SDL_CreateGPUTexture(device_, &tci);
        HIROMI_ASSERT(white_tex_ != nullptr, "Renderer2D: failed to create white texture");

        // Upload a single white pixel.
        SDL_GPUTransferBufferCreateInfo tbci{};
        tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbci.size  = 4;
        SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbci);
        auto* px = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device_, tb, false));
        px[0] = 255; px[1] = 255; px[2] = 255; px[3] = 255;
        SDL_UnmapGPUTransferBuffer(device_, tb);

        SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(device_);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(upload_cmd);
        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = tb;
        SDL_GPUTextureRegion dst{};
        dst.texture = white_tex_;
        dst.w = 1; dst.h = 1; dst.d = 1;
        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(upload_cmd);
        SDL_ReleaseGPUTransferBuffer(device_, tb);
    }

    // Create a default nearest-neighbor sampler.
    {
        SDL_GPUSamplerCreateInfo sci{};
        sci.min_filter   = SDL_GPU_FILTER_NEAREST;
        sci.mag_filter   = SDL_GPU_FILTER_NEAREST;
        sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sci.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        default_sampler_ = SDL_CreateGPUSampler(device_, &sci);
        HIROMI_ASSERT(default_sampler_ != nullptr, "Renderer2D: failed to create default sampler");
    }

    proj_ = glm::orthoRH_ZO(0.0f, static_cast<float>(fb_w_),
                             static_cast<float>(fb_h_), 0.0f,
                             -1.0f, 1.0f);
    log::info("Renderer2D initialized ({}x{})", fb_w_, fb_h_);
    return true;
}

void Renderer2D::shutdown() {
    if (!device_) return;

    SDL_WaitForGPUIdle(device_);

    pipeline_cache_.reset();

    if (vert_shader_) { SDL_ReleaseGPUShader(device_, vert_shader_); vert_shader_ = nullptr; }
    if (frag_shader_) { SDL_ReleaseGPUShader(device_, frag_shader_); frag_shader_ = nullptr; }
    if (depth_tex_)   { SDL_ReleaseGPUTexture(device_, depth_tex_);  depth_tex_   = nullptr; }
    if (vertex_buf_)  { SDL_ReleaseGPUBuffer(device_, vertex_buf_);   vertex_buf_  = nullptr; }
    if (index_buf_)   { SDL_ReleaseGPUBuffer(device_, index_buf_);    index_buf_   = nullptr; }
    if (white_tex_)   { SDL_ReleaseGPUTexture(device_, white_tex_);   white_tex_   = nullptr; }
    if (default_sampler_) { SDL_ReleaseGPUSampler(device_, default_sampler_); default_sampler_ = nullptr; }

    if (owns_device_) {
        SDL_DestroyGPUDevice(device_);
    }
    device_ = nullptr;
    log::info("Renderer2D shutdown");
}

void Renderer2D::begin_frame() {
    HIROMI_ASSERT(device_ != nullptr, "Renderer2D not initialized");
    cmd_buf_ = SDL_AcquireGPUCommandBuffer(device_);
    HIROMI_ASSERT(cmd_buf_ != nullptr, "Renderer2D: SDL_AcquireGPUCommandBuffer failed");
    draw_queue_.clear();
}

void Renderer2D::end_frame() {
    HIROMI_ASSERT(cmd_buf_ != nullptr, "Renderer2D::end_frame called without begin_frame");

    // Sort sprites by z_order so lower z draws first.
    std::stable_sort(draw_queue_.begin(), draw_queue_.end(),
        [](const SpriteDraw& a, const SpriteDraw& b) { return a.z_order < b.z_order; });

    flush_sprites(cmd_buf_);

    SDL_SubmitGPUCommandBuffer(cmd_buf_);
    cmd_buf_ = nullptr;
}

void Renderer2D::on_resize(int new_w, int new_h) {
    if (new_w == fb_w_ && new_h == fb_h_) return;
    fb_w_ = new_w;
    fb_h_ = new_h;

    if (device_) {
        SDL_WaitForGPUIdle(device_);
        recreate_depth_texture(fb_w_, fb_h_);
    }

    proj_ = glm::orthoRH_ZO(0.0f, static_cast<float>(fb_w_),
                             static_cast<float>(fb_h_), 0.0f,
                             -1.0f, 1.0f);
}

std::string_view Renderer2D::backend_name() const noexcept {
    return "SDL_GPU/2D";
}

void Renderer2D::submit_sprite(const SpriteDraw& draw) {
    draw_queue_.push_back(draw);
}

void Renderer2D::set_view_size(float width, float height) {
    proj_ = glm::orthoRH_ZO(0.0f, width, height, 0.0f, -1.0f, 1.0f);
}

// ---- Private ----------------------------------------------------------------

void Renderer2D::recreate_depth_texture(int w, int h) {
    if (depth_tex_) {
        SDL_ReleaseGPUTexture(device_, depth_tex_);
        depth_tex_ = nullptr;
    }

    SDL_GPUTextureCreateInfo info{};
    info.type             = SDL_GPU_TEXTURETYPE_2D;
    info.format           = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.usage            = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.width            = static_cast<uint32_t>(w);
    info.height           = static_cast<uint32_t>(h);
    info.layer_count_or_depth = 1;
    info.num_levels       = 1;

    depth_tex_ = SDL_CreateGPUTexture(device_, &info);
    HIROMI_ASSERT(depth_tex_ != nullptr, "Renderer2D: failed to create depth texture");
}

void Renderer2D::build_pipeline() {
    ShaderLoader loader(device_);

    vert_shader_ = loader.load(
        shader_path("sprite.vert"),
        SDL_GPU_SHADERSTAGE_VERTEX,
        preferred_shader_format(),
        "main",
        /*samplers=*/0,
        /*uniform_buffers=*/1);  // binding 0: projection matrix

    frag_shader_ = loader.load(
        shader_path("sprite.frag"),
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        preferred_shader_format(),
        "main",
        /*samplers=*/1);  // binding 0: texture sampler

    if (!vert_shader_ || !frag_shader_) {
        log::error("Renderer2D: failed to load shaders — sprites will not render");
        return;
    }

    // Vertex attribute layout matching SpriteVertex: xy uv rgba
    SDL_GPUVertexBufferDescription vbd{};
    vbd.slot             = 0;
    vbd.pitch            = sizeof(SpriteVertex);
    vbd.input_rate       = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[3] = {};
    attrs[0] = { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(SpriteVertex, x) };
    attrs[1] = { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(SpriteVertex, u) };
    attrs[2] = { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(SpriteVertex, r) };

    SDL_GPUColorTargetDescription ctd{};
    ctd.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
    ctd.blend_state.enable_blend       = true;
    ctd.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    ctd.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    ctd.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    ctd.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pi{};
    pi.vertex_shader   = vert_shader_;
    pi.fragment_shader = frag_shader_;
    pi.vertex_input_state.vertex_buffer_descriptions = &vbd;
    pi.vertex_input_state.num_vertex_buffers         = 1;
    pi.vertex_input_state.vertex_attributes          = attrs;
    pi.vertex_input_state.num_vertex_attributes      = 3;
    pi.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pi.target_info.color_target_descriptions     = &ctd;
    pi.target_info.num_color_targets             = 1;
    pi.target_info.depth_stencil_format          = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    pi.target_info.has_depth_stencil_target      = true;
    pi.depth_stencil_state.enable_depth_test     = false; // sprites use z_order, not depth buffer
    pi.depth_stencil_state.enable_depth_write    = false;

    PipelineKey key{};
    key.color_format = ctd.format;
    key.depth_test   = false;
    key.depth_write  = false;

    pipeline_ = pipeline_cache_->get_or_create(key, pi);
}

void Renderer2D::flush_sprites(SDL_GPUCommandBuffer* cmd_buf) {
    if (draw_queue_.empty()) {
        // Still need to do a clear pass.
        SDL_GPUTexture* swap_tex = nullptr;
        uint32_t sw = 0, sh = 0;
        if (!SDL_AcquireGPUSwapchainTexture(cmd_buf, window_, &swap_tex, &sw, &sh)
            || !swap_tex) return;

        SDL_GPUColorTargetInfo cti{};
        cti.texture     = swap_tex;
        cti.load_op     = SDL_GPU_LOADOP_CLEAR;
        cti.store_op    = SDL_GPU_STOREOP_STORE;
        cti.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};

        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd_buf, &cti, 1, nullptr);
        SDL_EndGPURenderPass(pass);
        return;
    }

    // --- Upload vertex + index data ---
    const uint32_t sprite_count = static_cast<uint32_t>(draw_queue_.size());
    const uint32_t vert_bytes   = sprite_count * 4 * sizeof(SpriteVertex);
    const uint32_t idx_bytes    = sprite_count * 6 * sizeof(uint16_t);

    // Grow buffers if needed.
    if (sprite_count > buf_capacity_sprites_) {
        if (vertex_buf_) SDL_ReleaseGPUBuffer(device_, vertex_buf_);
        if (index_buf_)  SDL_ReleaseGPUBuffer(device_, index_buf_);

        SDL_GPUBufferCreateInfo vbi{};
        vbi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbi.size  = sprite_count * 4 * sizeof(SpriteVertex);
        vertex_buf_ = SDL_CreateGPUBuffer(device_, &vbi);

        SDL_GPUBufferCreateInfo ibi{};
        ibi.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibi.size  = sprite_count * 6 * sizeof(uint16_t);
        index_buf_ = SDL_CreateGPUBuffer(device_, &ibi);

        buf_capacity_sprites_ = sprite_count;
    }

    // Transfer buffer for upload.
    SDL_GPUTransferBufferCreateInfo tbci{};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size  = vert_bytes + idx_bytes;
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbci);

    auto* map = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device_, tb, false));
    auto* verts   = reinterpret_cast<SpriteVertex*>(map);
    auto* indices = reinterpret_cast<uint16_t*>(map + vert_bytes);

    for (uint32_t i = 0; i < sprite_count; ++i) {
        const auto& draw = draw_queue_[i];
        verts[i * 4 + 0] = draw.verts[0]; // TL
        verts[i * 4 + 1] = draw.verts[1]; // TR
        verts[i * 4 + 2] = draw.verts[2]; // BL
        verts[i * 4 + 3] = draw.verts[3]; // BR
        // Two triangles per quad: TL-TR-BL, TR-BR-BL
        const uint16_t base = static_cast<uint16_t>(i * 4);
        indices[i * 6 + 0] = base + 0;
        indices[i * 6 + 1] = base + 1;
        indices[i * 6 + 2] = base + 2;
        indices[i * 6 + 3] = base + 1;
        indices[i * 6 + 4] = base + 3;
        indices[i * 6 + 5] = base + 2;
    }
    SDL_UnmapGPUTransferBuffer(device_, tb);

    // Issue a copy pass to upload.
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buf);
    SDL_GPUTransferBufferLocation src_loc{};
    src_loc.transfer_buffer = tb;

    SDL_GPUBufferRegion dst_region{};
    dst_region.buffer = vertex_buf_;
    dst_region.size   = vert_bytes;
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);

    src_loc.offset    = vert_bytes;
    dst_region.buffer = index_buf_;
    dst_region.size   = idx_bytes;
    SDL_UploadToGPUBuffer(copy, &src_loc, &dst_region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_ReleaseGPUTransferBuffer(device_, tb);

    // --- Render pass ---
    SDL_GPUTexture* swap_tex = nullptr;
    uint32_t sw = 0, sh = 0;
    if (!SDL_AcquireGPUSwapchainTexture(cmd_buf, window_, &swap_tex, &sw, &sh)
        || !swap_tex) return;

    SDL_GPUColorTargetInfo cti{};
    cti.texture     = swap_tex;
    cti.load_op     = SDL_GPU_LOADOP_CLEAR;
    cti.store_op    = SDL_GPU_STOREOP_STORE;
    cti.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd_buf, &cti, 1, nullptr);
    if (!pipeline_) { SDL_EndGPURenderPass(pass); return; }

    SDL_BindGPUGraphicsPipeline(pass, pipeline_);

    // Push projection matrix as uniform.
    SDL_PushGPUVertexUniformData(cmd_buf, 0, glm::value_ptr(proj_), sizeof(glm::mat4));

    SDL_GPUBufferBinding vb_bind{vertex_buf_, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vb_bind, 1);

    SDL_GPUBufferBinding ib_bind{index_buf_, 0};
    SDL_BindGPUIndexBuffer(pass, &ib_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Batch draws by texture to minimize binds.
    SDL_GPUTexture* last_tex = nullptr;
    uint32_t batch_start = 0;

    auto flush_batch = [&](uint32_t end_idx) {
        if (end_idx <= batch_start) return;
        SDL_DrawGPUIndexedPrimitives(pass,
            (end_idx - batch_start) * 6,
            1,
            batch_start * 6,
            static_cast<int32_t>(batch_start * 4),
            0);
    };

    // Bind fallback white texture before the loop so untextured sprites
    // at the start of the queue still have a valid sampler binding.
    {
        SDL_GPUTextureSamplerBinding tsb{white_tex_, default_sampler_};
        SDL_BindGPUFragmentSamplers(pass, 0, &tsb, 1);
    }

    for (uint32_t i = 0; i < sprite_count; ++i) {
        SDL_GPUTexture* tex = draw_queue_[i].texture;
        if (tex != last_tex) {
            flush_batch(i);
            batch_start = i;
            last_tex = tex;
            SDL_GPUTextureSamplerBinding tsb{tex ? tex : white_tex_, default_sampler_};
            SDL_BindGPUFragmentSamplers(pass, 0, &tsb, 1);
        }
    }
    flush_batch(sprite_count);

    SDL_EndGPURenderPass(pass);
}

} // namespace hiromi
