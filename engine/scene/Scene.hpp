#pragma once

#include "../ecs/EntityId.hpp"
#include <string>
#include <vector>

namespace hiromi {

class World;

// A Scene is a named group of EntityIds. It provides convenience methods for
// bulk-creating and bulk-destroying related entities (e.g., one scene per level).
// v1 stub: no serialization or streaming.
class Scene {
public:
    explicit Scene(std::string name);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    void register_entity(EntityId id);
    void unregister_entity(EntityId id);

    // Destroys all registered entities in the given world and clears the list.
    void clear(World& world);

    [[nodiscard]] const std::vector<EntityId>& entities() const noexcept { return entities_; }
    [[nodiscard]] std::size_t entity_count() const noexcept { return entities_.size(); }

private:
    std::string            name_;
    std::vector<EntityId>  entities_;
};

} // namespace hiromi
