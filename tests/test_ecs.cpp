#include <gtest/gtest.h>
#include <engine/ecs/World.hpp>
#include <engine/components/Transform.hpp>
#include <engine/components/RigidBody2D.hpp>

using namespace hiromi;

// ---- Entity lifecycle -------------------------------------------------------

TEST(ECS, CreateAndDestroyEntity) {
    World world;
    EntityId e = world.create_entity();
    EXPECT_TRUE(e.is_valid());
    EXPECT_TRUE(world.is_alive(e));

    world.destroy_entity(e);
    EXPECT_FALSE(world.is_alive(e));
}

TEST(ECS, DestroyedEntitySlotIsReused) {
    World world;
    EntityId e1 = world.create_entity();
    world.destroy_entity(e1);

    EntityId e2 = world.create_entity();
    // Same slot, different generation.
    EXPECT_EQ(e2.index, e1.index);
    EXPECT_NE(e2.generation, e1.generation);

    // Old handle must be dead.
    EXPECT_FALSE(world.is_alive(e1));
    EXPECT_TRUE(world.is_alive(e2));
}

TEST(ECS, NullEntityIsNotAlive) {
    World world;
    EXPECT_FALSE(world.is_alive(NULL_ENTITY));
}

// ---- Component add / get / has / remove ------------------------------------

TEST(ECS, AddAndGetComponent) {
    World world;
    EntityId e = world.create_entity();

    Transform t{};
    t.position = {1.0f, 2.0f, 3.0f};
    world.add_component(e, t);

    EXPECT_TRUE(world.has_component<Transform>(e));
    EXPECT_FLOAT_EQ(world.get_component<Transform>(e).position.x, 1.0f);
    EXPECT_FLOAT_EQ(world.get_component<Transform>(e).position.y, 2.0f);
}

TEST(ECS, RemoveComponent) {
    World world;
    EntityId e = world.create_entity();
    world.add_component(e, Transform{});
    EXPECT_TRUE(world.has_component<Transform>(e));

    world.remove_component<Transform>(e);
    EXPECT_FALSE(world.has_component<Transform>(e));
}

TEST(ECS, AddMultipleComponents) {
    World world;
    EntityId e = world.create_entity();
    world.add_component(e, Transform{});
    world.add_component(e, RigidBody2D{});

    EXPECT_TRUE(world.has_component<Transform>(e));
    EXPECT_TRUE(world.has_component<RigidBody2D>(e));
}

// ---- Archetype migration / swap_remove correctness --------------------------

TEST(ECS, ArchetypeMigrationPreservesData) {
    World world;
    EntityId e = world.create_entity();

    Transform t{};
    t.position = {5.0f, 10.0f, 0.0f};
    world.add_component(e, t);

    // Add a second component — forces archetype migration.
    RigidBody2D rb{};
    rb.mass = 2.5f;
    world.add_component(e, rb);

    // Data from the first component must survive migration.
    EXPECT_FLOAT_EQ(world.get_component<Transform>(e).position.x, 5.0f);
    EXPECT_FLOAT_EQ(world.get_component<RigidBody2D>(e).mass,     2.5f);
}

TEST(ECS, SwapRemoveDoesNotCorruptSurvivingEntities) {
    World world;

    // Create three entities with the same archetype.
    EntityId e0 = world.create_entity();
    EntityId e1 = world.create_entity();
    EntityId e2 = world.create_entity();

    Transform t0{}; t0.position = {1.0f, 0.0f, 0.0f};
    Transform t1{}; t1.position = {2.0f, 0.0f, 0.0f};
    Transform t2{}; t2.position = {3.0f, 0.0f, 0.0f};

    world.add_component(e0, t0);
    world.add_component(e1, t1);
    world.add_component(e2, t2);

    // Destroy the middle entity — triggers a swap_remove.
    world.destroy_entity(e1);

    EXPECT_FALSE(world.is_alive(e1));
    EXPECT_TRUE(world.is_alive(e0));
    EXPECT_TRUE(world.is_alive(e2));

    // Surviving entities must have correct data.
    EXPECT_FLOAT_EQ(world.get_component<Transform>(e0).position.x, 1.0f);
    EXPECT_FLOAT_EQ(world.get_component<Transform>(e2).position.x, 3.0f);
}

// ---- Resources --------------------------------------------------------------

TEST(ECS, AddAndGetResource) {
    struct Config { int max_entities = 100; };
    World world;
    world.add_resource(Config{200});
    EXPECT_EQ(world.get_resource<Config>().max_entities, 200);
}

TEST(ECS, HasResource) {
    struct Tag {};
    World world;
    EXPECT_FALSE(world.has_resource<Tag>());
    world.add_resource(Tag{});
    EXPECT_TRUE(world.has_resource<Tag>());
}
