#include "RenderSystem2D.hpp"
#include "../ecs/World.hpp"
#include "../components/Transform.hpp"
#include "../components/Sprite.hpp"
#include "../renderer/Renderer2D.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace hiromi {

void RenderSystem2D::update(World& world, float /*dt*/) {
    world.query<Transform, Sprite>().each([&](EntityId /*e*/, Transform& t, Sprite& s) {
        // Extract 2D position/rotation/scale from world_matrix.
        // world_matrix[3] is the translation column.
        const float wx = t.world_matrix[3][0];
        const float wy = t.world_matrix[3][1];

        // Scale from the world_matrix diagonal (approximate for uniform scale).
        const float sx = s.width  * t.scale.x;
        const float sy = s.height * t.scale.y;

        // Half extents for the quad corners.
        const float hw = sx * 0.5f;
        const float hh = sy * 0.5f;

        // UV coordinates.
        float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
        if (s.src_w > 0.0f && s.src_h > 0.0f) {
            // Non-zero src rect: compute normalized UVs.
            // This requires knowing the texture dimensions — for now treat as 0..1
            // within the rect. Full UV normalization is done in the shader when
            // a proper texture atlas is used. For sprites with known pixel size,
            // populate src_* correctly and use a sampler with the right dimensions.
            u0 = s.src_x; v0 = s.src_y;
            u1 = s.src_x + s.src_w; v1 = s.src_y + s.src_h;
        }

        if (s.flip_x) std::swap(u0, u1);
        if (s.flip_y) std::swap(v0, v1);

        SpriteDraw draw{};
        draw.texture = s.texture;
        draw.z_order = s.z_order;

        // Quad vertices: TL, TR, BL, BR (pre-rotated by world_matrix's rotation)
        // For simplicity in v1, use axis-aligned quads (rotation handled by
        // TransformSystem writing world_matrix; 2D rotation is extracted below).
        // Extract 2D angle from the z-column of the rotation part.
        // For a pure 2D rotation about Z: mat[0][0] = cos, mat[0][1] = sin.
        const float cos_a = t.world_matrix[0][0];
        const float sin_a = t.world_matrix[0][1];

        auto rotate2d = [&](float lx, float ly) -> std::pair<float, float> {
            return { cos_a * lx - sin_a * ly + wx,
                     sin_a * lx + cos_a * ly + wy };
        };

        auto [tlx, tly] = rotate2d(-hw, -hh);
        auto [trx, try_] = rotate2d( hw, -hh);
        auto [blx, bly] = rotate2d(-hw,  hh);
        auto [brx, bry] = rotate2d( hw,  hh);

        draw.verts[0] = { tlx, tly, u0, v0, s.color_r, s.color_g, s.color_b, s.color_a };
        draw.verts[1] = { trx, try_, u1, v0, s.color_r, s.color_g, s.color_b, s.color_a };
        draw.verts[2] = { blx, bly, u0, v1, s.color_r, s.color_g, s.color_b, s.color_a };
        draw.verts[3] = { brx, bry, u1, v1, s.color_r, s.color_g, s.color_b, s.color_a };

        renderer_.submit_sprite(draw);
    });
}

} // namespace hiromi
