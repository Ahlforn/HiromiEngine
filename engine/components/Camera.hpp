#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace hiromi {

struct Camera {
    // Projection parameters.
    float fov_y_rad  = glm::radians(60.0f);
    float near_plane = 0.1f;
    float far_plane  = 1000.0f;
    bool  is_ortho   = false;

    // Ortho parameters (used when is_ortho == true).
    float ortho_width  = 16.0f;
    float ortho_height = 9.0f;

    // Computed by RenderSystem3D from the camera entity's Transform.
    glm::mat4 view       = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    // Only one Camera with is_active = true is used per frame.
    bool is_active = false;
};

} // namespace hiromi
