#include "Scene.hpp"
#include "../ecs/World.hpp"
#include <algorithm>

namespace hiromi {

Scene::Scene(std::string name) : name_(std::move(name)) {}

void Scene::register_entity(EntityId id) {
    entities_.push_back(id);
}

void Scene::unregister_entity(EntityId id) {
    auto it = std::find(entities_.begin(), entities_.end(), id);
    if (it != entities_.end()) {
        *it = entities_.back();
        entities_.pop_back();
    }
}

void Scene::clear(World& world) {
    for (EntityId id : entities_) {
        if (world.is_alive(id)) {
            world.destroy_entity(id);
        }
    }
    entities_.clear();
}

} // namespace hiromi
