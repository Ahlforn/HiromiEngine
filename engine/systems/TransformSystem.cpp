#include "TransformSystem.hpp"
#include "../ecs/World.hpp"
#include "../components/Transform.hpp"
#include "../core/Assert.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <unordered_set>
#include <vector>

namespace hiromi {

static glm::mat4 compute_local_matrix(const Transform& t) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), t.position);
    m *= glm::mat4_cast(t.rotation);
    m  = glm::scale(m, t.scale);
    return m;
}

void TransformSystem::update(World& world, float /*dt*/) {
    // Collect all entities with Transform in a single pass.
    struct Entry { EntityId id; Transform* t; };
    std::vector<Entry> all;
    all.reserve(256);

    world.query<Transform>().each([&](EntityId id, Transform& t) {
        all.push_back({id, &t});
    });

    // --- Debug: detect cycles ---
#ifndef NDEBUG
    std::unordered_set<uint64_t> visited;
    auto entity_key = [](EntityId e) -> uint64_t {
        return (static_cast<uint64_t>(e.generation) << 32) | e.index;
    };
#endif

    // Propagate: visit roots first, then recurse into children.
    // For simplicity in v1, do two passes:
    //   Pass 1: update all root entities (parent == NULL_ENTITY)
    //   Pass 2: update non-root entities using parent's world_matrix
    // This is O(depth) passes for deep hierarchies; for v1 this is acceptable.

    bool changed = true;
    int  pass    = 0;
    while (changed && pass < 64) {
        changed = false;
        ++pass;
        for (auto& [id, t] : all) {
            if (!t->dirty) continue;

            if (!t->parent.is_valid()) {
                // Root: world_matrix = local_matrix
                t->world_matrix = compute_local_matrix(*t);
                t->dirty        = false;
                changed         = true;
            } else if (world.is_alive(t->parent) &&
                       world.has_component<Transform>(t->parent)) {
                const Transform& parent_t = world.get_component<Transform>(t->parent);
                if (!parent_t.dirty) {
                    // Parent is resolved; compute child world_matrix.
                    t->world_matrix = parent_t.world_matrix * compute_local_matrix(*t);
                    t->dirty        = false;
                    changed         = true;
                }
                // else: parent still dirty — will be resolved in a later pass.
            } else {
                // Parent gone or has no Transform: treat as root.
                t->world_matrix = compute_local_matrix(*t);
                t->dirty        = false;
                changed         = true;
            }
        }
    }

    HIROMI_ASSERT(pass < 64, "TransformSystem: hierarchy depth exceeded 64 — possible cycle");
}

} // namespace hiromi
