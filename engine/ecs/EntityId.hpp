#pragma once

#include <cstdint>
#include <functional>

namespace hiromi {

struct EntityId {
    static constexpr uint32_t INVALID_INDEX = 0xFFFF'FFFFu;
    static constexpr uint32_t INVALID_GEN   = 0xFFFF'FFFFu;

    uint32_t index      = INVALID_INDEX;
    uint32_t generation = INVALID_GEN;

    [[nodiscard]] bool is_valid() const noexcept {
        return index != INVALID_INDEX;
    }

    bool operator==(const EntityId&) const noexcept = default;
    bool operator!=(const EntityId&) const noexcept = default;
};

inline constexpr EntityId NULL_ENTITY{};

} // namespace hiromi

template<>
struct std::hash<hiromi::EntityId> {
    std::size_t operator()(const hiromi::EntityId& e) const noexcept {
        // Combine two 32-bit values into one 64-bit hash.
        uint64_t v = (static_cast<uint64_t>(e.generation) << 32u) | e.index;
        v ^= v >> 33u;
        v *= 0xff51afd7ed558ccdull;
        v ^= v >> 33u;
        v *= 0xc4ceb9fe1a85ec53ull;
        v ^= v >> 33u;
        return static_cast<std::size_t>(v);
    }
};
