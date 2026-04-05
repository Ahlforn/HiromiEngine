#pragma once

#include <string_view>

namespace hiromi {

class World;

class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void update(World& world, float dt) = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

} // namespace hiromi
