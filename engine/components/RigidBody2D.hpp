#pragma once

#include <glm/vec2.hpp>

namespace hiromi {

struct RigidBody2D {
    glm::vec2 velocity     = glm::vec2(0.0f);
    glm::vec2 acceleration = glm::vec2(0.0f);  // reset each frame after integration

    float mass        = 1.0f;
    float restitution = 0.3f;  // bounciness coefficient [0, 1]
    float linear_drag = 0.0f;  // velocity damping per second

    bool is_static    = false;  // static bodies don't move
    bool use_gravity  = true;

    // AABB half-extents in world units for collision.
    float half_width  = 0.5f;
    float half_height = 0.5f;
};

} // namespace hiromi
