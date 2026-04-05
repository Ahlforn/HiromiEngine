#pragma once

#include "../ecs/EntityId.hpp"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace hiromi {

struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale    = glm::vec3(1.0f);

    // Optional parent entity. NULL_ENTITY = root (world space).
    EntityId parent = NULL_ENTITY;

    // Computed by TransformSystem from position/rotation/scale + parent chain.
    // Do not set directly.
    glm::mat4 world_matrix = glm::mat4(1.0f);

    // Set to true when position/rotation/scale changes. TransformSystem clears it.
    bool dirty = true;

    // Convenience: local-space TRS matrix (ignores parent).
    [[nodiscard]] glm::mat4 local_matrix() const noexcept {
        glm::mat4 m = glm::mat4_cast(rotation);
        m[3] = glm::vec4(position, 1.0f);
        return glm::scale(glm::translate(glm::mat4(1.0f), position)
                          * glm::mat4_cast(rotation), scale);
    }
};

} // namespace hiromi
