#include "RenderSystem3D.hpp"
#include "../ecs/World.hpp"
#include "../components/Transform.hpp"
#include "../components/Mesh.hpp"
#include "../components/Camera.hpp"
#include "../renderer/Renderer3D.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace hiromi {

void RenderSystem3D::update(World& world, float /*dt*/) {
    // --- Find the active camera ---
    glm::mat4 view(1.0f), proj(1.0f);
    bool found_camera = false;

    world.query<Transform, Camera>().each([&](EntityId /*e*/, Transform& t, Camera& cam) {
        if (!cam.is_active || found_camera) return;
        found_camera = true;

        // View matrix: inverse of the camera's world transform.
        view = glm::inverse(t.world_matrix);

        // Projection matrix.
        // fb_w/h are available via renderer; use camera's own params here.
        if (cam.is_ortho) {
            const float hw = cam.ortho_width  * 0.5f;
            const float hh = cam.ortho_height * 0.5f;
            proj = glm::orthoRH_ZO(-hw, hw, -hh, hh,
                                   cam.near_plane, cam.far_plane);
        } else {
            // Aspect ratio is renderer-owned; use 16/9 as a fallback.
            // RenderSystem3D could be extended to receive fb dimensions from Engine.
            const float aspect = 16.0f / 9.0f;
            proj = glm::perspectiveRH_ZO(cam.fov_y_rad, aspect,
                                         cam.near_plane, cam.far_plane);
        }

        // Store back into Camera for other systems/debugging.
        cam.view       = view;
        cam.projection = proj;
    });

    if (!found_camera) return;

    renderer_.set_view_projection(view, proj);

    // --- Submit mesh draw commands ---
    world.query<Transform, Mesh>().each([&](EntityId /*e*/, Transform& t, Mesh& m) {
        if (!m.vertex_buffer || !m.index_buffer) return;

        DrawCommand3D cmd{};
        cmd.vertex_buffer = m.vertex_buffer;
        cmd.index_buffer  = m.index_buffer;
        cmd.index_count   = m.index_count;
        cmd.first_index   = m.index_offset;
        cmd.vertex_offset = static_cast<int32_t>(m.vertex_offset);
        cmd.pipeline_id   = m.pipeline_id;
        cmd.model_matrix  = t.world_matrix;

        renderer_.submit_draw(cmd);
    });
}

} // namespace hiromi
