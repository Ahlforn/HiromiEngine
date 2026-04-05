#pragma once

#include "EntityId.hpp"
#include "Archetype.hpp"
#include "ArchetypeStorage.hpp"
#include "ComponentTypeId.hpp"
#include "Query.hpp"
#include "../core/Assert.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <cassert>
#include <stdexcept>

namespace hiromi {

// Per-entity bookkeeping stored in a flat array indexed by EntityId::index.
struct EntityRecord {
    uint32_t          generation = 0;
    ArchetypeStorage* storage    = nullptr;
    std::size_t       row        = 0;
    bool              alive      = false;
};

// World is the root container for all ECS state. It owns every ArchetypeStorage,
// the entity record array, and all global resources.
//
// Thread safety: World is NOT thread-safe. All access must be from one thread,
// or externally synchronized.
class World {
public:
    World();
    ~World() = default;

    World(const World&)            = delete;
    World& operator=(const World&) = delete;

    // -------------------------------------------------------------------------
    // Entity lifecycle
    // -------------------------------------------------------------------------

    [[nodiscard]] EntityId create_entity();
    void                   destroy_entity(EntityId id);

    [[nodiscard]] bool is_alive(EntityId id) const noexcept;

    // -------------------------------------------------------------------------
    // Component mutation — each call causes archetype migration
    // -------------------------------------------------------------------------

    template<typename T>
    void add_component(EntityId id, T component);

    template<typename T>
    void remove_component(EntityId id);

    template<typename T>
    [[nodiscard]] T& get_component(EntityId id);

    template<typename T>
    [[nodiscard]] const T& get_component(EntityId id) const;

    template<typename T>
    [[nodiscard]] bool has_component(EntityId id) const noexcept;

    // -------------------------------------------------------------------------
    // Query
    // -------------------------------------------------------------------------

    // Returns a Query<C...> iterating all archetypes that have all of C...
    template<typename... C>
        requires (Component<C> && ...)
    [[nodiscard]] Query<C...> query();

    // -------------------------------------------------------------------------
    // Resources (global non-entity data)
    // -------------------------------------------------------------------------

    template<typename T>
    T& add_resource(T resource);

    template<typename T>
    [[nodiscard]] T& get_resource();

    template<typename T>
    [[nodiscard]] const T& get_resource() const;

    template<typename T>
    [[nodiscard]] bool has_resource() const noexcept;

    template<typename T>
    void remove_resource();

private:
    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    EntityRecord&       record_of(EntityId id);
    const EntityRecord& record_of(EntityId id) const;

    ArchetypeStorage& get_or_create_storage(const Archetype& arch);

    // Migrate entity to a new archetype. Returns new row.
    std::size_t migrate(EntityId id, const Archetype& new_arch);

    // -------------------------------------------------------------------------
    // Data
    // -------------------------------------------------------------------------

    std::vector<EntityRecord> entity_records_;
    std::vector<uint32_t>     free_list_;

    // Archetype table registry
    std::unordered_map<Archetype, std::unique_ptr<ArchetypeStorage>, Archetype::Hash> storages_;

    // The empty archetype storage holds freshly-created entities (no components).
    ArchetypeStorage* empty_storage_ = nullptr;

    // Type-erased resource map: ResourceRegistry::get<T>() → {ptr, deleter}
    struct ResourceEntry {
        std::unique_ptr<void, void(*)(void*)> ptr;
    };
    std::unordered_map<ComponentTypeId, ResourceEntry> resources_;

#ifndef NDEBUG
    bool iterating_ = false; // prevents structural mutations inside each()
#endif
};

} // namespace hiromi

// Template implementations (must be visible at call sites)
#include "World.inl"
