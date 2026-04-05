#include "ArchetypeStorage.hpp"
#include <cassert>

namespace hiromi {

ArchetypeStorage::ArchetypeStorage(const Archetype& archetype)
    : archetype_(archetype) {}

std::size_t ArchetypeStorage::allocate_row(EntityId entity) {
    std::size_t row = entities_.size();
    entities_.push_back(entity);
    // Push uninitialized slots in every pool.
    for (auto& [id, pool] : pools_) {
        pool->push_back_uninitialized();
    }
    return row;
}

std::size_t ArchetypeStorage::migrate_from(ArchetypeStorage& source,
                                            std::size_t       source_row,
                                            EntityId          entity) {
    std::size_t new_row = entities_.size();
    entities_.push_back(entity);

    for (auto& [id, dst_pool] : pools_) {
        // Allocate slot in destination.
        void* dst = dst_pool->push_back_uninitialized();

        // If the source archetype also has this component, move it over.
        ComponentPool* src_pool = source.pool_for(id);
        if (src_pool) {
            dst_pool->move_into(new_row, src_pool->at(source_row));
        }
        // If source doesn't have it, the slot is uninitialized — caller fills it.
    }

    return new_row;
}

EntityId ArchetypeStorage::remove_row(std::size_t row) {
    assert(row < entities_.size());

    std::size_t last = entities_.size() - 1;
    EntityId    displaced = (row != last) ? entities_[last] : NULL_ENTITY;

    // Swap-remove in every pool.
    for (auto& [id, pool] : pools_) {
        pool->swap_remove(row);
    }

    // Swap-remove entity bookkeeping.
    if (row != last) {
        entities_[row] = entities_[last];
    }
    entities_.pop_back();

    return displaced;
}

ComponentPool* ArchetypeStorage::pool_for(ComponentTypeId id) noexcept {
    auto it = pools_.find(id);
    return (it != pools_.end()) ? it->second.get() : nullptr;
}

const ComponentPool* ArchetypeStorage::pool_for(ComponentTypeId id) const noexcept {
    auto it = pools_.find(id);
    return (it != pools_.end()) ? it->second.get() : nullptr;
}

} // namespace hiromi
