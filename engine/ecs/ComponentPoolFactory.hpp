#pragma once

#include "ComponentTypeId.hpp"
#include "ComponentPool.hpp"
#include <unordered_map>
#include <functional>
#include <memory>

namespace hiromi {

// Global registry mapping ComponentTypeId → factory function that creates the
// correct ComponentPool for that type. Populated lazily when add_component<T>
// is first called for a type T.
class ComponentPoolFactory {
public:
    using FactoryFn = std::function<std::unique_ptr<ComponentPool>()>;

    static ComponentPoolFactory& instance() noexcept {
        static ComponentPoolFactory inst;
        return inst;
    }

    template<typename T>
    void ensure_registered() {
        const ComponentTypeId id = ComponentRegistry::get<T>();
        if (factories_.find(id) == factories_.end()) {
            factories_[id] = [] { return ComponentPool::create<T>(); };
        }
    }

    // Creates a new ComponentPool for the given type ID.
    // Returns nullptr if the type was never registered (logic bug).
    [[nodiscard]] std::unique_ptr<ComponentPool> create(ComponentTypeId id) const {
        auto it = factories_.find(id);
        if (it == factories_.end()) return nullptr;
        return it->second();
    }

    [[nodiscard]] bool is_registered(ComponentTypeId id) const noexcept {
        return factories_.find(id) != factories_.end();
    }

private:
    ComponentPoolFactory() = default;
    std::unordered_map<ComponentTypeId, FactoryFn> factories_;
};

} // namespace hiromi
