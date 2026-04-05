#include <engine/engine/Engine.hpp>
#include <engine/components/Transform.hpp>
#include <engine/components/Sprite.hpp>
#include <engine/components/RigidBody2D.hpp>
#include <engine/systems/InputSystem.hpp>
#include <engine/systems/PhysicsSystem2D.hpp>
#include <engine/systems/AnimationSystem.hpp>
#include <engine/systems/TransformSystem.hpp>
#include <engine/systems/RenderSystem2D.hpp>
#include <engine/input/InputState.hpp>

using namespace hiromi;

// Example gameplay system: move the square with WASD and quit on Escape.
struct PlayerMoveSystem final : ISystem {
    void update(World& world, float /*dt*/) override {
        if (!world.has_resource<InputState>()) return;
        const InputState& input = world.get_resource<InputState>();

        const float speed = 4.0f; // pixels per tick at 60 Hz

        world.query<Transform, RigidBody2D>().each(
            [&](EntityId /*e*/, Transform& /*t*/, RigidBody2D& rb) {
                rb.velocity = {0.0f, 0.0f};
                if (input.key(SDL_SCANCODE_D) || input.key(SDL_SCANCODE_RIGHT)) rb.velocity.x =  speed * 60.0f;
                if (input.key(SDL_SCANCODE_A) || input.key(SDL_SCANCODE_LEFT))  rb.velocity.x = -speed * 60.0f;
                if (input.key(SDL_SCANCODE_W) || input.key(SDL_SCANCODE_UP))    rb.velocity.y = -speed * 60.0f;
                if (input.key(SDL_SCANCODE_S) || input.key(SDL_SCANCODE_DOWN))  rb.velocity.y =  speed * 60.0f;
            });
    }
    std::string_view name() const noexcept override { return "PlayerMoveSystem"; }
};

int main() {
    EngineConfig cfg;
    cfg.title      = "Hiromi Engine — Hello World";
    cfg.width      = 1280;
    cfg.height     = 720;
    cfg.enable_2d  = true;
    cfg.enable_3d  = false;
    cfg.physics_hz = 60.0f;

    Engine engine(cfg);
    if (!engine.initialize()) return 1;

    // --- Systems (fixed tick: input → gameplay → physics → animation) ---
    engine.add_system<InputSystem>(SystemSchedule::FIXED, engine.running_flag());
    engine.add_system<PlayerMoveSystem>(SystemSchedule::FIXED);
    engine.add_system<PhysicsSystem2D>(SystemSchedule::FIXED, /*gravity=*/0.0f);
    engine.add_system<AnimationSystem>(SystemSchedule::FIXED);

    // --- Systems (variable tick: transform → render) ---
    engine.add_system<TransformSystem>(SystemSchedule::VARIABLE);
    engine.add_system<RenderSystem2D>(SystemSchedule::VARIABLE, *engine.renderer_2d());

    // --- Entities ---
    World& world = engine.world();

    // A blue square in the center.
    EntityId square = world.create_entity();

    Transform t{};
    t.position = {640.0f, 360.0f, 0.0f};
    world.add_component(square, t);

    Sprite s{};
    s.width   = 64.0f;
    s.height  = 64.0f;
    s.color_r = 0.2f;
    s.color_g = 0.6f;
    s.color_b = 1.0f;
    world.add_component(square, s);

    RigidBody2D rb{};
    rb.use_gravity = false;
    rb.half_width  = 32.0f;
    rb.half_height = 32.0f;
    world.add_component(square, rb);

    engine.run();
    return 0;
}
