# CLAUDE.md — Hiromi Engine

## Project Overview

**Hiromi Engine** is a C++20 game engine with an archetype-based ECS core, SDL3
for windowing/input/rendering, and vcpkg for package management. It supports both
2D (orthographic) and 3D (perspective) rendering through a unified SDL_GPU pipeline.

Full architecture reference: `docs/architecture.md`

---

## Build

```bash
# Prerequisites: VCPKG_ROOT set, Ninja installed, Vulkan SDK for glslc
cmake --preset default        # Debug build
cmake --preset release        # Release build
cmake --build build

# Compile shaders (must be done before running; .spv files go in assets/shaders/compiled/)
glslc assets/shaders/src/sprite.vert.glsl -o assets/shaders/compiled/sprite.vert.spv
glslc assets/shaders/src/sprite.frag.glsl -o assets/shaders/compiled/sprite.frag.spv
glslc assets/shaders/src/mesh.vert.glsl   -o assets/shaders/compiled/mesh.vert.spv
glslc assets/shaders/src/mesh.frag.glsl   -o assets/shaders/compiled/mesh.frag.spv

./build/example/hiromi_example

# Tests (enable with -DHIROMI_BUILD_TESTS=ON)
cmake --preset default -DHIROMI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

---

## Directory Structure

```
engine/
  core/          Types.hpp, Assert.hpp, Logger.hpp
  ecs/           EntityId, ComponentTypeId, Archetype, ComponentPool,
                 ArchetypeStorage, World, Query, System
  components/    Transform, Sprite, Mesh, Camera, RigidBody2D,
                 AnimationState, AudioSource
  renderer/      IRenderer, Renderer2D, Renderer3D, GPUPipelineCache, ShaderLoader
  input/         InputState
  systems/       InputSystem, TransformSystem, RenderSystem2D, RenderSystem3D,
                 PhysicsSystem2D, AnimationSystem
  scene/         Scene (named entity group)
  engine/        Engine (main loop, system dispatch)
example/         main.cpp — hello-world blue square
assets/shaders/  GLSL sources (src/) and compiled SPIR-V (compiled/)
tests/           GTest suites: test_ecs.cpp, test_query.cpp
```

---

## ECS Core Concepts

### Entity IDs
`struct EntityId { uint32_t index; uint32_t generation; }` — O(1) lookup; stale
handles detected by generation mismatch.

### Archetype-based storage
All entities with the same component set share one `ArchetypeStorage` (a table).
Each column in the table is a `ComponentPool` — a dense, type-erased array for
one component type (SoA layout). This gives cache-friendly iteration.

### Adding/removing components
Triggers **archetype migration**: entity data is moved from the old table to a new
one. `ComponentPoolFactory::ensure_registered<T>()` is called by `add_component`
to register the pool factory before a new archetype is constructed.

### Query
```cpp
world.query<Transform, Sprite>().each([](EntityId e, Transform& t, Sprite& s) {
    // iterate all entities that have AT LEAST Transform and Sprite
});
```
Structural mutations (`add_component`, `remove_component`, `destroy_entity`) inside
an `each` callback are **undefined behaviour** — asserted in debug builds.

### Resources
Global, non-entity data stored via `world.add_resource<T>()` /
`world.get_resource<T>()`. Uses a separate ID counter from components.
`InputState` is the main built-in resource.

---

## Renderer

Both `Renderer2D` and `Renderer3D` use **SDL_GPU only**. `SDL_Renderer` is never
used — it cannot coexist with `SDL_GPU` on the same window.

- **Renderer2D**: orthographic projection, sprite quad batching, blended draw calls
  sorted by `Sprite::z_order`.
- **Renderer3D**: perspective pipeline, per-frame draw queue, depth buffer,
  `GPUPipelineCache` for pipeline reuse.

Command buffer lifecycle per frame:
1. `begin_frame()` — `SDL_AcquireGPUCommandBuffer`
2. Systems call `submit_sprite` / `submit_draw`
3. `end_frame()` — flush → render pass → `SDL_SubmitGPUCommandBuffer`

Depth texture is recreated on every `on_resize`.

### NDC / GLM
SDL_GPU uses Vulkan/Metal NDC (Y-down, depth [0,1]). Configured via:
```cpp
// Set in engine/CMakeLists.txt — do not add manually
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
```
Use `glm::perspectiveRH_ZO` and `glm::orthoRH_ZO` everywhere. Never use the plain
`glm::perspective` / `glm::ortho` variants — they produce incorrect results with
SDL_GPU.

### Shaders
GLSL sources in `assets/shaders/src/`. Compiled offline to SPIR-V with `glslc`.
The `#pragma shader_stage(vertex/fragment)` directive is used in some shaders
for SDL_shadercross compatibility.

Shader uniform binding conventions:
- Vertex uniform buffer set 1, binding 0 — projection / MVP matrix
- Fragment sampler set 2, binding 0 — texture sampler

---

## Game Loop

Fixed timestep accumulator at `config.physics_hz` (default 60 Hz).

```
Fixed systems  (called N times per frame until accumulator is drained):
  InputSystem → [gameplay systems] → PhysicsSystem2D → AnimationSystem

Variable systems (called once per rendered frame):
  TransformSystem → RenderSystem2D / RenderSystem3D
```

Register systems via:
```cpp
engine.add_system<InputSystem>(SystemSchedule::FIXED, engine.running_flag());
engine.add_system<TransformSystem>(SystemSchedule::VARIABLE);
engine.add_system<RenderSystem2D>(SystemSchedule::VARIABLE, *engine.renderer_2d());
```

---

## Adding a New Component

1. Create a header-only struct in `engine/components/MyComponent.hpp`.
2. Ensure its move constructor is `noexcept` (required by `ComponentPool`).
3. No registration step needed — `add_component<MyComponent>` auto-registers
   the pool factory on first call.

## Adding a New System

1. Create `engine/systems/MySystem.hpp` inheriting `ISystem`.
2. Implement `void update(World&, float dt)` and `std::string_view name()`.
3. Add the `.cpp` to `engine/CMakeLists.txt` source list.
4. Register in user code: `engine.add_system<MySystem>(schedule, args...)`.

---

## Known Gotchas

| # | Issue | Mitigation |
|---|---|---|
| 1 | `swap_remove` cascade | World immediately updates displaced entity's `EntityRecord::row` |
| 2 | SDL_GPU + SDL_Renderer window conflict | SDL_GPU only — no SDL_Renderer |
| 3 | Depth texture resize | Destroyed and recreated in `on_resize` |
| 4 | GLM NDC mismatch | `GLM_FORCE_DEPTH_ZERO_TO_ONE` set in CMake; use `*RH_ZO` variants |
| 5 | Mutation inside `each` | UB; `iterating_` assert in debug |
| 6 | Transform hierarchy cycles | Multi-pass loop capped at 64 iterations; assert fires on overflow |
| 7 | Resource vs component ID collision | `ResourceRegistry` uses a separate counter |
| 8 | noexcept move requirement | `static_assert` in `World::add_component<T>` |

---

## Dependencies (vcpkg)

| Package | Use |
|---|---|
| `sdl3` | Window, input, GPU rendering |
| `sdl3-image` (png, jpeg) | Image loading |
| `sdl3-ttf` | Font rendering |
| `glm` | Math (vectors, matrices, quaternions) |
| `fmt` | Logging and string formatting |
