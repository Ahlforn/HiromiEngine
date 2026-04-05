#pragma once

#include "../ecs/System.hpp"

namespace hiromi {

class Renderer2D;

// Queries Transform + Sprite, builds SpriteDraw commands, submits to Renderer2D.
class RenderSystem2D final : public ISystem {
public:
    explicit RenderSystem2D(Renderer2D& renderer) : renderer_(renderer) {}

    void update(World& world, float dt) override;
    [[nodiscard]] std::string_view name() const noexcept override { return "RenderSystem2D"; }

private:
    Renderer2D& renderer_;
};

} // namespace hiromi
