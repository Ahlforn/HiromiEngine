#include "PhysicsSystem2D.hpp"
#include "../ecs/World.hpp"
#include "../components/Transform.hpp"
#include "../components/RigidBody2D.hpp"

#include <vector>
#include <cmath>

namespace hiromi {

void PhysicsSystem2D::update(World& world, float dt) {
    integrate(world, dt);
    resolve_collisions(world);
}

void PhysicsSystem2D::integrate(World& world, float dt) {
    world.query<Transform, RigidBody2D>().each(
        [&](EntityId /*e*/, Transform& t, RigidBody2D& rb) {
            if (rb.is_static) return;

            // Apply gravity.
            if (rb.use_gravity) {
                rb.acceleration.y += gravity_y_;
            }

            // Integrate: semi-implicit Euler (velocity first, then position).
            rb.velocity += rb.acceleration * dt;

            // Apply linear drag.
            if (rb.linear_drag > 0.0f) {
                rb.velocity *= std::max(0.0f, 1.0f - rb.linear_drag * dt);
            }

            t.position.x += rb.velocity.x * dt;
            t.position.y += rb.velocity.y * dt;
            t.dirty = true;

            // Reset per-frame acceleration (external forces re-apply each tick).
            rb.acceleration = glm::vec2(0.0f);
        });
}

// Simple AABB vs AABB collision resolution (minimum translation vector).
void PhysicsSystem2D::resolve_collisions(World& world) {
    struct CollidableEntry {
        EntityId   id;
        Transform* transform;
        RigidBody2D* body;
    };

    std::vector<CollidableEntry> entities;
    entities.reserve(64);

    world.query<Transform, RigidBody2D>().each(
        [&](EntityId id, Transform& t, RigidBody2D& rb) {
            entities.push_back({id, &t, &rb});
        });

    const auto n = entities.size();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            auto& a = entities[i];
            auto& b = entities[j];

            if (a.body->is_static && b.body->is_static) continue;

            const float ax = a.transform->position.x;
            const float ay = a.transform->position.y;
            const float bx = b.transform->position.x;
            const float by = b.transform->position.y;

            const float ahw = a.body->half_width;
            const float ahh = a.body->half_height;
            const float bhw = b.body->half_width;
            const float bhh = b.body->half_height;

            const float dx = bx - ax;
            const float dy = by - ay;
            const float overlap_x = (ahw + bhw) - std::abs(dx);
            const float overlap_y = (ahh + bhh) - std::abs(dy);

            if (overlap_x <= 0.0f || overlap_y <= 0.0f) continue; // no overlap

            // Resolve along the axis of minimum penetration.
            if (overlap_x < overlap_y) {
                const float sign = (dx > 0.0f) ? 1.0f : -1.0f;
                const float correction = overlap_x * 0.5f;

                if (!a.body->is_static) {
                    a.transform->position.x -= sign * correction;
                    a.body->velocity.x = -a.body->velocity.x * a.body->restitution;
                    a.transform->dirty = true;
                }
                if (!b.body->is_static) {
                    b.transform->position.x += sign * correction;
                    b.body->velocity.x = -b.body->velocity.x * b.body->restitution;
                    b.transform->dirty = true;
                }
            } else {
                const float sign = (dy > 0.0f) ? 1.0f : -1.0f;
                const float correction = overlap_y * 0.5f;

                if (!a.body->is_static) {
                    a.transform->position.y -= sign * correction;
                    a.body->velocity.y = -a.body->velocity.y * a.body->restitution;
                    a.transform->dirty = true;
                }
                if (!b.body->is_static) {
                    b.transform->position.y += sign * correction;
                    b.body->velocity.y = -b.body->velocity.y * b.body->restitution;
                    b.transform->dirty = true;
                }
            }
        }
    }
}

} // namespace hiromi
