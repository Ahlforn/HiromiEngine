#pragma once

#include <cstdint>
#include <atomic>

namespace hiromi {

using ComponentTypeId = uint32_t;

class ComponentRegistry {
public:
    template<typename T>
    static ComponentTypeId get() noexcept {
        static const ComponentTypeId id = next_id();
        return id;
    }

    static uint32_t registered_count() noexcept {
        return counter_.load(std::memory_order_relaxed);
    }

private:
    static ComponentTypeId next_id() noexcept {
        return counter_.fetch_add(1u, std::memory_order_relaxed);
    }

    inline static std::atomic<uint32_t> counter_{0u};
};

// Separate counter for resources so they can never collide with component IDs.
class ResourceRegistry {
public:
    template<typename T>
    static ComponentTypeId get() noexcept {
        static const ComponentTypeId id = next_id();
        return id;
    }

private:
    static ComponentTypeId next_id() noexcept {
        return counter_.fetch_add(1u, std::memory_order_relaxed);
    }

    inline static std::atomic<uint32_t> counter_{0u};
};

} // namespace hiromi
