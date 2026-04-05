#pragma once

#include "../ecs/System.hpp"

namespace hiromi {

// Propagates parent→child transform hierarchies and recomputes world_matrix
// for any entity whose Transform is marked dirty.
//
// Processes root entities (parent == NULL_ENTITY) first, then recursively
// visits children. A visited-set guards against cycles in debug builds.
class TransformSystem final : public ISystem {
public:
    void update(World& world, float dt) override;
    [[nodiscard]] std::string_view name() const noexcept override { return "TransformSystem"; }
};

} // namespace hiromi
