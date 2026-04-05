// Included at the bottom of World.hpp. Do not include directly.
#pragma once

#include "ComponentPoolFactory.hpp"

namespace hiromi {

// ---- add_component ----------------------------------------------------------

template<typename T>
void World::add_component(EntityId id, T component) {
    static_assert(std::is_nothrow_move_constructible_v<T>,
        "All components must have noexcept move constructors.");

#ifndef NDEBUG
    HIROMI_ASSERT(!iterating_,
        "add_component called during query iteration — undefined behaviour.");
#endif
    HIROMI_ASSERT(is_alive(id), "add_component on dead entity");

    // Ensure pool factory knows how to create pools for T.
    ComponentPoolFactory::instance().ensure_registered<T>();

    const ComponentTypeId type_id = ComponentRegistry::get<T>();
    EntityRecord& rec = record_of(id);

    HIROMI_ASSERT(!rec.storage->archetype().has(type_id),
        "add_component: entity already has this component type.");

    Archetype new_arch = rec.storage->archetype().with(type_id);
    std::size_t new_row = migrate(id, new_arch);

    // Place the new component into its uninitialized slot.
    ArchetypeStorage& new_storage = *record_of(id).storage;
    ComponentPool* dst_pool = new_storage.pool_for(type_id);
    HIROMI_ASSERT(dst_pool != nullptr, "pool missing after migration");
    new (dst_pool->at(new_row)) T(std::move(component));
}

// ---- remove_component -------------------------------------------------------

template<typename T>
void World::remove_component(EntityId id) {
#ifndef NDEBUG
    HIROMI_ASSERT(!iterating_,
        "remove_component called during query iteration — undefined behaviour.");
#endif
    HIROMI_ASSERT(is_alive(id), "remove_component on dead entity");

    const ComponentTypeId type_id = ComponentRegistry::get<T>();
    const EntityRecord& rec = record_of(id);
    HIROMI_ASSERT(rec.storage->archetype().has(type_id),
        "remove_component: entity does not have this component type.");

    // migrate_from only copies components present in both archetypes. T is absent
    // from new_arch, so it won't be moved. The old slot is then removed via
    // swap_remove in migrate(), which calls the pool's destructor_ — properly
    // destructing T without a manual ~T() call here.
    Archetype new_arch = rec.storage->archetype().without(type_id);
    migrate(id, new_arch);
}

// ---- get_component ----------------------------------------------------------

template<typename T>
T& World::get_component(EntityId id) {
    HIROMI_ASSERT(is_alive(id), "get_component on dead entity");
    const EntityRecord& rec = record_of(id);
    HIROMI_ASSERT(rec.storage->archetype().has(ComponentRegistry::get<T>()),
        "get_component: entity does not have this component type.");
    return rec.storage->get<T>(rec.row);
}

template<typename T>
const T& World::get_component(EntityId id) const {
    HIROMI_ASSERT(is_alive(id), "get_component on dead entity");
    const EntityRecord& rec = record_of(id);
    HIROMI_ASSERT(rec.storage->archetype().has(ComponentRegistry::get<T>()),
        "get_component: entity does not have this component type.");
    return rec.storage->get<T>(rec.row);
}

// ---- has_component ----------------------------------------------------------

template<typename T>
bool World::has_component(EntityId id) const noexcept {
    if (!is_alive(id)) return false;
    return record_of(id).storage->archetype().has(ComponentRegistry::get<T>());
}

// ---- query ------------------------------------------------------------------

template<typename... C>
    requires (Component<C> && ...)
Query<C...> World::query() {
    Archetype required({ComponentRegistry::get<C>()...});
    std::vector<ArchetypeStorage*> matching;
    matching.reserve(8);

    for (auto& [arch, storage] : storages_) {
        if (arch.is_superset_of(required)) {
            matching.push_back(storage.get());
        }
    }

#ifndef NDEBUG
    // Set iterating_ for the duration of the Query's each() call.
    // We do it here so the caller doesn't need to think about it.
    // Query holds a raw pointer to iterating_ for the guard.
    // Simpler: just set it in query() and clear in Query destructor.
    // For now, the guard is in each() via a lambda capture.
    (void)iterating_;
#endif

    return Query<C...>(std::move(matching));
}

// ---- resources --------------------------------------------------------------

template<typename T>
T& World::add_resource(T resource) {
    const ComponentTypeId rid = ResourceRegistry::get<T>();
    HIROMI_ASSERT(resources_.find(rid) == resources_.end(),
        "add_resource: resource of this type already exists.");

    auto* heap = new T(std::move(resource));
    ResourceEntry entry{
        std::unique_ptr<void, void(*)(void*)>(heap, [](void* p) {
            delete static_cast<T*>(p);
        })
    };
    resources_.emplace(rid, std::move(entry));
    return *heap;
}

template<typename T>
T& World::get_resource() {
    const ComponentTypeId rid = ResourceRegistry::get<T>();
    auto it = resources_.find(rid);
    HIROMI_ASSERT(it != resources_.end(), "get_resource: resource not found.");
    return *static_cast<T*>(it->second.ptr.get());
}

template<typename T>
const T& World::get_resource() const {
    const ComponentTypeId rid = ResourceRegistry::get<T>();
    auto it = resources_.find(rid);
    HIROMI_ASSERT(it != resources_.end(), "get_resource: resource not found.");
    return *static_cast<const T*>(it->second.ptr.get());
}

template<typename T>
bool World::has_resource() const noexcept {
    return resources_.find(ResourceRegistry::get<T>()) != resources_.end();
}

template<typename T>
void World::remove_resource() {
    resources_.erase(ResourceRegistry::get<T>());
}

} // namespace hiromi
