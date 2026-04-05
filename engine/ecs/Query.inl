// Included at the bottom of Query.hpp once ArchetypeStorage is fully defined.
// Do not include this file directly.

#pragma once

#include "ArchetypeStorage.hpp"

namespace hiromi {

// Helper: maps any type to ComponentPool*, making the pointer type dependent
// on a parameter pack so that pack expansion works correctly.
template<typename> using PoolPtr = ComponentPool*;

template<typename... C>
    requires (Component<C> && ...)
template<typename Fn>
    requires std::invocable<Fn, EntityId, C&...>
void Query<C...>::each(Fn&& fn) {
    for (ArchetypeStorage* storage : storages_) {
        if (storage->entity_count() == 0) continue;

        // Grab typed pool pointers once per archetype.
        std::tuple<PoolPtr<C>...> pools{
            storage->pool_for(ComponentRegistry::get<C>())...
        };

        // Debug: assert every requested pool exists in this archetype.
        std::apply([](PoolPtr<C>... p) {
            ((void)(assert(p != nullptr)), ...);
        }, pools);

        const std::size_t         count    = storage->entity_count();
        const std::vector<EntityId>& entities = storage->entities();

        for (std::size_t row = 0; row < count; ++row) {
            // Unpack pool pointers and call fn with typed references.
            std::apply([&](PoolPtr<C>... p) {
                fn(entities[row], *static_cast<C*>(p->at(row))...);
            }, pools);
        }
    }
}

template<typename... C>
    requires (Component<C> && ...)
template<typename Fn>
    requires std::invocable<Fn, C&...>
void Query<C...>::each_component(Fn&& fn) {
    each([&fn](EntityId, C&... components) {
        fn(components...);
    });
}

} // namespace hiromi
