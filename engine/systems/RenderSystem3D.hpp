#pragma once

#include "../ecs/System.hpp"

namespace hiromi {

class Renderer3D;

// Finds the active Camera, queries Transform + Mesh entities, and submits
// DrawCommand3D structs to Renderer3D.
class RenderSystem3D final : public ISystem {
public:
    explicit RenderSystem3D(Renderer3D& renderer) : renderer_(renderer) {}

    void update(World& world, float dt) override;
    [[nodiscard]] std::string_view name() const noexcept override { return "RenderSystem3D"; }

private:
    Renderer3D& renderer_;
};

} // namespace hiromi
