#include <gtest/gtest.h>
#include <engine/ecs/World.hpp>
#include <engine/components/Transform.hpp>
#include <engine/components/Sprite.hpp>
#include <engine/components/RigidBody2D.hpp>

using namespace hiromi;

// ---- Basic query iteration --------------------------------------------------

TEST(Query, IteratesMatchingEntities) {
    World world;

    EntityId e0 = world.create_entity();
    EntityId e1 = world.create_entity();
    EntityId e2 = world.create_entity();

    world.add_component(e0, Transform{});
    world.add_component(e1, Transform{});
    world.add_component(e2, Transform{});

    int count = 0;
    world.query<Transform>().each([&](EntityId, Transform&) { ++count; });
    EXPECT_EQ(count, 3);
}

TEST(Query, DoesNotIterateEntitiesMissingComponent) {
    World world;

    EntityId e0 = world.create_entity();
    EntityId e1 = world.create_entity();

    world.add_component(e0, Transform{});
    // e1 has no Transform.

    int count = 0;
    world.query<Transform>().each([&](EntityId, Transform&) { ++count; });
    EXPECT_EQ(count, 1);
}

TEST(Query, MultiComponentQueryMatchesSuperset) {
    World world;

    // e0: Transform + Sprite (matches Transform+Sprite query)
    // e1: Transform only     (does NOT match)
    EntityId e0 = world.create_entity();
    EntityId e1 = world.create_entity();

    world.add_component(e0, Transform{});
    world.add_component(e0, Sprite{});
    world.add_component(e1, Transform{});

    int count = 0;
    world.query<Transform, Sprite>().each([&](EntityId, Transform&, Sprite&) { ++count; });
    EXPECT_EQ(count, 1);
}

// ---- Query spans multiple archetypes ----------------------------------------

TEST(Query, SpansMultipleArchetypes) {
    World world;

    // Three archetypes containing Transform:
    //   A: Transform
    //   B: Transform + Sprite
    //   C: Transform + RigidBody2D

    EntityId a = world.create_entity();
    world.add_component(a, Transform{});

    EntityId b = world.create_entity();
    world.add_component(b, Transform{});
    world.add_component(b, Sprite{});

    EntityId c = world.create_entity();
    world.add_component(c, Transform{});
    world.add_component(c, RigidBody2D{});

    int count = 0;
    world.query<Transform>().each([&](EntityId, Transform&) { ++count; });
    EXPECT_EQ(count, 3);
}

// ---- Query count ------------------------------------------------------------

TEST(Query, CountMatchesIterationCount) {
    World world;
    for (int i = 0; i < 10; ++i) {
        EntityId e = world.create_entity();
        world.add_component(e, Transform{});
    }

    auto q = world.query<Transform>();
    EXPECT_EQ(q.count(), 10u);

    int iterated = 0;
    q.each([&](EntityId, Transform&) { ++iterated; });
    EXPECT_EQ(iterated, 10);
}

// ---- Entity ID stability after swap_remove ----------------------------------

TEST(Query, EntityIdCorrectAfterSwapRemove) {
    World world;

    EntityId e0 = world.create_entity();
    EntityId e1 = world.create_entity();
    EntityId e2 = world.create_entity();

    Transform t0{}; t0.position = {10.0f, 0.0f, 0.0f};
    Transform t1{}; t1.position = {20.0f, 0.0f, 0.0f};
    Transform t2{}; t2.position = {30.0f, 0.0f, 0.0f};

    world.add_component(e0, t0);
    world.add_component(e1, t1);
    world.add_component(e2, t2);

    world.destroy_entity(e1); // triggers swap_remove: e2 moves to e1's slot

    // Query must now return e0 and e2 with correct data.
    std::vector<std::pair<EntityId, float>> results;
    world.query<Transform>().each([&](EntityId id, Transform& t) {
        results.push_back({id, t.position.x});
    });

    ASSERT_EQ(results.size(), 2u);

    // Sort by entity index for deterministic comparison.
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.first.index < b.first.index; });

    // e0 should have position 10, e2 should have position 30.
    bool found_e0 = false, found_e2 = false;
    for (auto& [id, x] : results) {
        if (id == e0) { EXPECT_FLOAT_EQ(x, 10.0f); found_e0 = true; }
        if (id == e2) { EXPECT_FLOAT_EQ(x, 30.0f); found_e2 = true; }
    }
    EXPECT_TRUE(found_e0);
    EXPECT_TRUE(found_e2);
}
