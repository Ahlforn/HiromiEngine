#include "World.hpp"
#include "ComponentPoolFactory.hpp"
#include "../core/Assert.hpp"

namespace hiromi {

World::World() {
    // Create the empty archetype storage (entities with no components start here).
    Archetype empty_arch{};
    auto storage = std::make_unique<ArchetypeStorage>(empty_arch);
    empty_storage_ = storage.get();
    storages_[empty_arch] = std::move(storage);
}

// ---- Entity lifecycle -------------------------------------------------------

EntityId World::create_entity() {
    uint32_t index;
    uint32_t gen;

    if (!free_list_.empty()) {
        index = free_list_.back();
        free_list_.pop_back();
        gen = entity_records_[index].generation; // already incremented on destroy
    } else {
        index = static_cast<uint32_t>(entity_records_.size());
        entity_records_.emplace_back();
        gen = 0;
    }

    EntityRecord& rec = entity_records_[index];
    rec.generation = gen;
    rec.alive      = true;
    rec.storage    = empty_storage_;
    rec.row        = empty_storage_->allocate_row({index, gen});

    return EntityId{index, gen};
}

void World::destroy_entity(EntityId id) {
    HIROMI_ASSERT(is_alive(id), "destroy_entity: entity is already dead");

    EntityRecord& rec = record_of(id);
    EntityId displaced = rec.storage->remove_row(rec.row);

    // Update the entity that was swap-removed into this slot.
    if (displaced.is_valid()) {
        entity_records_[displaced.index].row = rec.row;
    }

    // Invalidate the record and recycle the slot.
    rec.alive      = false;
    rec.storage    = nullptr;
    rec.generation++;           // invalidates any stale EntityId with old gen
    free_list_.push_back(id.index);
}

bool World::is_alive(EntityId id) const noexcept {
    if (!id.is_valid()) return false;
    if (id.index >= entity_records_.size()) return false;
    const auto& rec = entity_records_[id.index];
    return rec.alive && rec.generation == id.generation;
}

// ---- Internal helpers -------------------------------------------------------

EntityRecord& World::record_of(EntityId id) {
    HIROMI_ASSERT(is_alive(id), "record_of: entity is dead");
    return entity_records_[id.index];
}

const EntityRecord& World::record_of(EntityId id) const {
    HIROMI_ASSERT(is_alive(id), "record_of: entity is dead");
    return entity_records_[id.index];
}

ArchetypeStorage& World::get_or_create_storage(const Archetype& arch) {
    auto it = storages_.find(arch);
    if (it != storages_.end()) return *it->second;

    // Create new storage and register pools for every component type.
    auto storage = std::make_unique<ArchetypeStorage>(arch);
    auto& factory = ComponentPoolFactory::instance();

    for (ComponentTypeId cid : arch.types()) {
        HIROMI_ASSERT(factory.is_registered(cid),
            "get_or_create_storage: component type has no registered pool factory. "
            "Make sure add_component<T> is called before querying archetypes containing T.");
        auto pool = factory.create(cid);
        // Register the raw pool into the storage.
        // We reach into the storage here because we need type-erased insertion.
        storage->register_pool_raw(cid, std::move(pool));
    }

    auto* ptr = storage.get();
    storages_[arch] = std::move(storage);
    return *ptr;
}

std::size_t World::migrate(EntityId id, const Archetype& new_arch) {
    EntityRecord& rec          = record_of(id);
    ArchetypeStorage& old_storage = *rec.storage;
    ArchetypeStorage& new_storage = get_or_create_storage(new_arch);

    std::size_t new_row = new_storage.migrate_from(old_storage, rec.row, id);

    // Remove from old storage (swap_remove).
    EntityId displaced = old_storage.remove_row(rec.row);

    // Update the entity that was displaced by swap_remove.
    if (displaced.is_valid()) {
        entity_records_[displaced.index].row = rec.row;
    }

    // Update this entity's record.
    rec.storage = &new_storage;
    rec.row     = new_row;

    return new_row;
}

} // namespace hiromi
