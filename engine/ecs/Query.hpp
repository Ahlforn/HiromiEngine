#pragma once

#include "ComponentTypeId.hpp"
#include "EntityId.hpp"
#include <vector>
#include <span>
#include <concepts>

namespace hiromi {

// Forward declaration — full definition in ArchetypeStorage.hpp
class ArchetypeStorage;
class ComponentPool;

// Concept: a valid component type must be nothrow-move-constructible.
template<typename T>
concept Component = std::is_nothrow_move_constructible_v<T>;

// Compile-time typed view across all archetypes that contain all of C...
//
// Usage:
//   world.query<Transform, Sprite>().each([](EntityId e, Transform& t, Sprite& s) {
//       // ...
//   });
//
// IMPORTANT: Calling add_component / remove_component / destroy_entity inside
// an each() callback is undefined behaviour. In debug builds the World sets an
// iterating_ flag to assert against this.
template<typename... C>
    requires (Component<C> && ...)
class Query {
public:
    explicit Query(std::vector<ArchetypeStorage*> storages)
        : storages_(std::move(storages)) {}

    // Iterate all matching entities, calling fn(EntityId, C&...) for each.
    template<typename Fn>
        requires std::invocable<Fn, EntityId, C&...>
    void each(Fn&& fn);

    // Iterate without the EntityId parameter.
    template<typename Fn>
        requires std::invocable<Fn, C&...>
    void each_component(Fn&& fn);

    [[nodiscard]] std::size_t count() const noexcept {
        std::size_t n = 0;
        for (auto* s : storages_) n += s->entity_count();
        return n;
    }

    [[nodiscard]] std::span<ArchetypeStorage* const> storages() const noexcept {
        return storages_;
    }

private:
    std::vector<ArchetypeStorage*> storages_;
};

} // namespace hiromi

// Implementation is in a .inl file to break the include cycle:
//   Query.hpp → (needs) ArchetypeStorage.hpp → (needs) ComponentPool.hpp
// Query.inl is included AFTER ArchetypeStorage.hpp is fully visible.
// World.hpp includes both, so users only need World.hpp.
#include "Query.inl"
