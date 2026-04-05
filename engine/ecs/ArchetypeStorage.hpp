#pragma once

#include "Archetype.hpp"
#include "ComponentPool.hpp"
#include "EntityId.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <cassert>

namespace hiromi {

// One archetype table: all entities with exactly the same component set.
// Rows are parallel across all pools: pool[i].at(row) == same entity's data.
class ArchetypeStorage {
public:
    explicit ArchetypeStorage(const Archetype& archetype);

    // Register a typed pool for component T. Must be called once per type
    // during construction. Type must match one of the archetype's component IDs.
    template<typename T>
    void register_pool(ComponentTypeId id) {
        assert(archetype_.has(id));
        assert(pools_.find(id) == pools_.end());
        pools_[id] = ComponentPool::create<T>();
    }

    // Type-erased pool registration used by World::get_or_create_storage.
    void register_pool_raw(ComponentTypeId id, std::unique_ptr<ComponentPool> pool) {
        assert(archetype_.has(id));
        assert(pools_.find(id) == pools_.end());
        pools_[id] = std::move(pool);
    }

    // Allocates a new uninitialized row for entity. Returns the new row index.
    // Caller must fill all component pools at this row before the row is used.
    std::size_t allocate_row(EntityId entity);

    // Moves all components shared between source.archetype and this archetype
    // from source_row into a new row in this storage. Returns new row index.
    // Components present in this archetype but NOT in source must be set by the
    // caller after this returns (e.g., the newly added component).
    std::size_t migrate_from(ArchetypeStorage& source, std::size_t source_row, EntityId entity);

    // Removes a row via swap_remove. Returns the EntityId of the entity that
    // was in the last row and is now at `row` (NULL_ENTITY if row was already
    // last). Caller must update that entity's row in the World entity map.
    EntityId remove_row(std::size_t row);

    // Typed component access.
    template<typename T>
    T& get(std::size_t row) {
        auto* pool = pool_for(ComponentRegistry::get<T>());
        assert(pool != nullptr);
        assert(row < pool->size());
        return *static_cast<T*>(pool->at(row));
    }

    template<typename T>
    const T& get(std::size_t row) const {
        const auto* pool = pool_for(ComponentRegistry::get<T>());
        assert(pool != nullptr);
        assert(row < pool->size());
        return *static_cast<const T*>(pool->at(row));
    }

    // Raw pool access for query iteration.
    [[nodiscard]] ComponentPool*       pool_for(ComponentTypeId id) noexcept;
    [[nodiscard]] const ComponentPool* pool_for(ComponentTypeId id) const noexcept;

    [[nodiscard]] const Archetype&            archetype()    const noexcept { return archetype_; }
    [[nodiscard]] std::size_t                 entity_count() const noexcept { return entities_.size(); }
    [[nodiscard]] const std::vector<EntityId>& entities()    const noexcept { return entities_; }

private:
    Archetype                                                     archetype_;
    std::unordered_map<ComponentTypeId, std::unique_ptr<ComponentPool>> pools_;
    std::vector<EntityId>                                         entities_;
};

} // namespace hiromi
