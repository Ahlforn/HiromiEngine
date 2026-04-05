#pragma once

#include "IRenderer.hpp"
#include "GPUPipelineCache.hpp"
#include "ShaderLoader.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/mat4x4.hpp>
#include <memory>
#include <vector>

namespace hiromi {

struct DrawCommand3D {
    SDL_GPUBuffer* vertex_buffer  = nullptr;
    SDL_GPUBuffer* index_buffer   = nullptr;
    uint32_t       index_count    = 0;
    uint32_t       first_index    = 0;
    int32_t        vertex_offset  = 0;
    uint32_t       pipeline_id    = 0;
    glm::mat4      model_matrix   = glm::mat4(1.0f);
};

// Per-object push constant: model+view+projection matrix (MVP)
struct MeshPushConstants {
    glm::mat4 mvp;
};

class Renderer3D final : public IRenderer {
public:
    Renderer3D() = default;
    ~Renderer3D() override;

    // Call when sharing device with Renderer2D (Engine creates the device once).
    bool initialize_with_device(SDL_Window* window, SDL_GPUDevice* device);

    // Creates its own device (standalone use).
    bool initialize(SDL_Window* window) override;

    void shutdown() override;
    void begin_frame() override;
    void end_frame()   override;
    void on_resize(int new_w, int new_h) override;

    [[nodiscard]] std::string_view backend_name() const noexcept override;

    // --- Draw API (call between begin_frame and end_frame) ---

    void submit_draw(const DrawCommand3D& cmd);

    // Set the view/projection matrices for this frame (from active Camera).
    void set_view_projection(const glm::mat4& view, const glm::mat4& proj);

    // --- GPU resource helpers ---

    // Creates a static vertex buffer and uploads data. Returns null on failure.
    [[nodiscard]] SDL_GPUBuffer* create_vertex_buffer(const void* data, uint32_t bytes);

    // Creates a static index buffer and uploads data.
    [[nodiscard]] SDL_GPUBuffer* create_index_buffer(const void* data, uint32_t bytes);

    void release_buffer(SDL_GPUBuffer*);

    [[nodiscard]] SDL_GPUDevice* gpu_device() const noexcept { return device_; }
    [[nodiscard]] GPUPipelineCache* pipeline_cache() const noexcept { return pipeline_cache_.get(); }

private:
    void recreate_depth_texture(int w, int h);
    void flush_draw_commands(SDL_GPUCommandBuffer* cmd_buf, SDL_GPUTexture* swap_tex);

    SDL_Window*    window_  = nullptr;
    SDL_GPUDevice* device_  = nullptr;
    bool           owns_device_ = false;

    SDL_GPUTexture*           depth_tex_   = nullptr;
    SDL_GPUCommandBuffer*     cmd_buf_     = nullptr; // valid between begin/end

    std::unique_ptr<GPUPipelineCache> pipeline_cache_;

    std::vector<DrawCommand3D> draw_queue_;

    glm::mat4 view_ = glm::mat4(1.0f);
    glm::mat4 proj_ = glm::mat4(1.0f);

    int fb_w_ = 0, fb_h_ = 0;
};

} // namespace hiromi
