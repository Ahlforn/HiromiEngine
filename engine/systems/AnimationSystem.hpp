#pragma once

#include "../ecs/System.hpp"

namespace hiromi {

// Advances sprite animation frames.
// Queries Sprite + AnimationState; updates Sprite::src_x based on current frame.
class AnimationSystem final : public ISystem {
public:
    void update(World& world, float dt) override;
    [[nodiscard]] std::string_view name() const noexcept override { return "AnimationSystem"; }
};

} // namespace hiromi
