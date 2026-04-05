#pragma once

#include "IRenderer.hpp"
#include "GPUPipelineCache.hpp"
#include "ShaderLoader.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <vector>

namespace hiromi {

// Sprite vertex layout (interleaved): position (xy), uv, color (rgba)
struct SpriteVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct SpriteDraw {
    SDL_GPUTexture* texture = nullptr;
    SpriteVertex    verts[4];    // quad: TL, TR, BL, BR
    int             z_order = 0;
};

// 2D renderer using SDL_GPU with an orthographic projection.
// Both Renderer2D and Renderer3D share the same SDL_GPUDevice (and window).
// Engine creates the device once and passes it to both renderers.
class Renderer2D final : public IRenderer {
public:
    Renderer2D() = default;
    ~Renderer2D() override;

    // device and window must outlive this renderer.
    // Call initialize() after the device has been claimed for the window.
    bool initialize(SDL_Window* window) override;
    bool initialize_with_device(SDL_Window* window, SDL_GPUDevice* device);

    void shutdown() override;
    void begin_frame() override;
    void end_frame()   override;
    void on_resize(int new_w, int new_h) override;

    [[nodiscard]] std::string_view backend_name() const noexcept override;

    // --- 2D draw API (call between begin_frame and end_frame) ---

    // Submit a sprite quad for drawing. Sorting by z_order happens in end_frame.
    void submit_sprite(const SpriteDraw& draw);

    // Set the orthographic projection size (world units). Default: window pixels.
    void set_view_size(float width, float height);

    [[nodiscard]] SDL_GPUDevice* gpu_device() const noexcept { return device_; }

private:
    void build_pipeline();
    void recreate_depth_texture(int w, int h);
    void flush_sprites(SDL_GPUCommandBuffer* cmd_buf);

    SDL_Window*      window_  = nullptr;
    SDL_GPUDevice*   device_  = nullptr;
    bool             owns_device_ = false;  // true if we created the device

    SDL_GPUTexture*  depth_tex_ = nullptr;
    SDL_GPUShader*   vert_shader_ = nullptr;
    SDL_GPUShader*   frag_shader_ = nullptr;

    std::unique_ptr<GPUPipelineCache> pipeline_cache_;
    SDL_GPUGraphicsPipeline*          pipeline_ = nullptr;

    // Dynamic vertex/index buffers (upload per frame)
    SDL_GPUBuffer* vertex_buf_ = nullptr;
    SDL_GPUBuffer* index_buf_  = nullptr;
    uint32_t       buf_capacity_sprites_ = 0;  // capacity in sprite quads

    std::vector<SpriteDraw> draw_queue_;

    glm::mat4 proj_  = glm::mat4(1.0f);
    int fb_w_ = 0, fb_h_ = 0;

    SDL_GPUCommandBuffer* cmd_buf_ = nullptr; // valid between begin/end_frame
};

} // namespace hiromi
