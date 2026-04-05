#pragma once

#include "../ecs/System.hpp"

namespace hiromi {

// Integrates RigidBody2D velocity/acceleration, applies gravity, and
// performs broad-phase + narrow-phase AABB collision detection.
// Receives the fixed timestep (1/60 s by default).
class PhysicsSystem2D final : public ISystem {
public:
    explicit PhysicsSystem2D(float gravity_y = 980.0f) : gravity_y_(gravity_y) {}

    void update(World& world, float dt) override;
    [[nodiscard]] std::string_view name() const noexcept override { return "PhysicsSystem2D"; }

    void set_gravity(float g) noexcept { gravity_y_ = g; }
    [[nodiscard]] float gravity() const noexcept { return gravity_y_; }

private:
    void integrate(World& world, float dt);
    void resolve_collisions(World& world);

    float gravity_y_ = 980.0f; // pixels/s² (positive = downward)
};

} // namespace hiromi
