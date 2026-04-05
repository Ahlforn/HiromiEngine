# Hiromi Engine

A C++20 game engine with an archetype-based ECS core, SDL3 for windowing/input/rendering, and vcpkg for package management. Supports both 2D (orthographic) and 3D (perspective) rendering through a unified SDL_GPU pipeline.

---

## Features

- **Archetype ECS** — cache-friendly SoA storage; O(1) entity lookup with generational handles
- **SDL_GPU rendering** — Metal on macOS, Vulkan elsewhere; no SDL_Renderer
- **2D & 3D** — orthographic sprite batching and perspective draw queue, same pipeline
- **Fixed timestep game loop** — 60 Hz physics/input, variable-rate rendering
- **Compile-time queries** — `world.query<A, B>().each(...)` with C++20 fold expressions
- **Shader cross-compilation** — GLSL → SPIR-V → MSL, built automatically by CMake
- **vcpkg dependencies** — reproducible builds, no manual library setup

---

## Prerequisites

| Requirement | Notes |
|---|---|
| C++20 compiler | Clang / GCC / MSVC |
| CMake ≥ 3.25 | With Ninja |
| vcpkg | `VCPKG_ROOT` env var must be set |
| Vulkan SDK (`glslc`) | GLSL → SPIR-V shader compilation |
| `spirv-cross` | SPIR-V → MSL for Metal (macOS) |

---

## Build

```bash
# Debug build (default preset)
cmake --preset default
cmake --build build

# Release build
cmake --preset release
cmake --build build

# Run the example
./build/example/hiromi_example

# Tests
cmake --preset default -DHIROMI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

Shader compilation (GLSL → SPIR-V → MSL) runs automatically as part of the build.

---

## Quick Start

```cpp
#include "engine/Engine.hpp"
#include "engine/components/Transform.hpp"
#include "engine/components/Sprite.hpp"
#include "engine/systems/RenderSystem2D.hpp"

int main() {
    Engine engine;

    // Register systems
    engine.add_system<InputSystem>(SystemSchedule::FIXED, engine.running_flag());
    engine.add_system<PhysicsSystem2D>(SystemSchedule::FIXED);
    engine.add_system<TransformSystem>(SystemSchedule::VARIABLE);
    engine.add_system<RenderSystem2D>(SystemSchedule::VARIABLE, *engine.renderer_2d());

    // Create an entity
    World& world = engine.world();
    EntityId e = world.create_entity();
    world.add_component<Transform>(e, {.position = {100, 200, 0}});
    world.add_component<Sprite>(e, {.texture = my_texture, .z_order = 0});

    // Query entities each frame
    world.query<Transform, Sprite>().each([](EntityId id, Transform& t, Sprite& s) {
        t.position.x += 1.0f;
    });

    engine.run();
}
```

---

## Architecture

### ECS

All entities sharing the same component set are stored in one `ArchetypeStorage` (a table). Each column is a `ComponentPool` — a dense, type-erased array (SoA layout) for cache-friendly iteration.

```
ArchetypeStorage: [Transform, Sprite]
  Pool<Transform>: [(10,20,0), (30,40,0), ...]
  Pool<Sprite>:    [tex1,      tex2,      ...]
  entities_:       [EntityId{0,1}, EntityId{1,1}, ...]
```

Adding or removing a component triggers an **archetype migration**: all existing component data is moved to a new storage and the old slot is freed via O(1) `swap_remove`.

Entity handles use a 32-bit generation counter — stale handles are detected immediately on access.

### Renderer

Both `Renderer2D` and `Renderer3D` use SDL_GPU exclusively. SDL_Renderer cannot coexist with SDL_GPU on the same window and is not used anywhere.

**Command buffer lifecycle per frame:**
1. `begin_frame()` — `SDL_AcquireGPUCommandBuffer`
2. Systems call `submit_sprite` / `submit_draw` → enqueued
3. `end_frame()` — flush → render pass → `SDL_SubmitGPUCommandBuffer`

**Shader pipeline:** GLSL sources live in `assets/shaders/src/`. CMake compiles them to SPIR-V (`.spv`) and cross-compiles to MSL (`.msl`) at build time. At runtime, `ShaderLoader` picks the correct format for the current platform.

### Game Loop

```
Fixed tick (60 Hz):
  InputSystem → PhysicsSystem2D → AnimationSystem

Variable tick (every rendered frame):
  TransformSystem → RenderSystem2D / RenderSystem3D
```

The accumulator is capped to prevent spiral-of-death on slow frames.

---

## Directory Structure

```
engine/
  core/        Types.hpp, Assert.hpp, Logger.hpp
  ecs/         EntityId, Archetype, ComponentPool, ArchetypeStorage, World, Query
  components/  Transform, Sprite, Mesh, Camera, RigidBody2D, AnimationState, AudioSource
  renderer/    IRenderer, Renderer2D, Renderer3D, GPUPipelineCache, ShaderLoader
  input/       InputState
  systems/     InputSystem, TransformSystem, RenderSystem2D, RenderSystem3D,
               PhysicsSystem2D, AnimationSystem
  scene/       Scene (named entity group)
  engine/      Engine (main loop, system dispatch)
example/       main.cpp — hello-world blue square
assets/shaders/
  src/         GLSL source files
  compiled/    SPIR-V (.spv) and MSL (.msl) output
tests/         GTest suites: test_ecs.cpp, test_query.cpp
docs/          architecture.md — detailed design reference
```

---

## Extending the Engine

### Adding a Component

1. Create a header-only struct in `engine/components/MyComponent.hpp`.
2. The move constructor **must** be `noexcept` (enforced by `static_assert`).
3. No registration needed — `world.add_component<MyComponent>(e, ...)` auto-registers on first call.

### Adding a System

1. Create `engine/systems/MySystem.hpp` inheriting `ISystem`.
2. Implement `void update(World&, float dt)` and `std::string_view name()`.
3. Add the `.cpp` to `engine/CMakeLists.txt`.
4. Register: `engine.add_system<MySystem>(SystemSchedule::FIXED_OR_VARIABLE, args...)`.

---

## Dependencies

| Package | Purpose |
|---|---|
| `sdl3` | Window, input, GPU rendering |
| `sdl3-image` | PNG / JPEG image loading |
| `sdl3-ttf` | Font rendering |
| `glm` | Vectors, matrices, quaternions |
| `fmt` | Logging and string formatting |

All installed automatically via vcpkg.
