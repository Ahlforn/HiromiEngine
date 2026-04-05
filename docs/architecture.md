# Hiromi Engine — Architecture

## Overview

Hiromi Engine is a C++20 game engine built around an **archetype-based ECS** core,
**SDL3** for windowing/input/rendering, and **vcpkg** for package management. It
supports both 2D and 3D rendering through a unified SDL_GPU pipeline.

---

## SDL_GPU-Only Rendering

`SDL_CreateRenderer` (SDL_Renderer) and `SDL_CreateGPUDevice` + `SDL_ClaimWindowForGPUDevice`
(SDL_GPU) are mutually exclusive on the same window. Both `Renderer2D` and `Renderer3D`
use **SDL_GPU only**. "2D" rendering is orthographic-projection quads on the same GPU
pipeline — no SDL_Renderer is used anywhere.

---

## Directory Structure

```
HiromiEngine/
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── vcpkg-configuration.json
├── docs/
│   └── architecture.md             # this file
│
├── engine/
│   ├── CMakeLists.txt
│   │
│   ├── core/
│   │   ├── Types.hpp               # u8, u32, f32, etc.
│   │   ├── Assert.hpp              # HIROMI_ASSERT macro
│   │   └── Logger.hpp              # fmt-based logging
│   │
│   ├── ecs/
│   │   ├── EntityId.hpp            # 32-bit index + 32-bit generation
│   │   ├── ComponentTypeId.hpp     # static template counter; no RTTI
│   │   ├── Archetype.hpp           # sorted ComponentTypeId set + Hash
│   │   ├── ComponentPool.hpp       # type-erased dense array (SoA column)
│   │   ├── ArchetypeStorage.hpp/.cpp  # one table per archetype; owns pools
│   │   ├── World.hpp/.cpp          # owns all storages; entity lifecycle
│   │   ├── Query.hpp + Query.inl   # compile-time typed iterator (fold expr)
│   │   └── System.hpp              # ISystem base (virtual update/name)
│   │
│   ├── components/                 # header-only data structs
│   │   ├── Transform.hpp           # pos/rot/scale + parent + world_matrix
│   │   ├── Sprite.hpp              # SDL_GPUTexture* + src_rect + z_order
│   │   ├── Mesh.hpp                # vertex/index buffer handles + pipeline_id
│   │   ├── Camera.hpp              # projection + view + fov + is_active
│   │   ├── RigidBody2D.hpp         # velocity + mass + restitution + is_static
│   │   ├── AnimationState.hpp      # frame/fps/elapsed/looping
│   │   └── AudioSource.hpp         # clip handle + volume + looping (stub)
│   │
│   ├── renderer/
│   │   ├── IRenderer.hpp           # begin_frame / end_frame / on_resize
│   │   ├── Renderer2D.hpp/.cpp     # SDL_GPU + orthographic helpers
│   │   ├── Renderer3D.hpp/.cpp     # SDL_GPU + perspective + draw queue
│   │   ├── GPUPipelineCache.hpp/.cpp  # key→SDL_GPUGraphicsPipeline cache
│   │   └── ShaderLoader.hpp/.cpp   # loads .spv blobs → SDL_GPUShader
│   │
│   ├── systems/
│   │   ├── InputSystem.hpp/.cpp    # SDL_PollEvent → InputState resource
│   │   ├── TransformSystem.hpp/.cpp # parent-child DFS → world_matrix
│   │   ├── RenderSystem2D.hpp/.cpp # Transform+Sprite → Renderer2D draw
│   │   ├── RenderSystem3D.hpp/.cpp # Transform+Mesh+Camera → Renderer3D draw
│   │   ├── PhysicsSystem2D.hpp/.cpp # integrate velocity + AABB collision
│   │   └── AnimationSystem.hpp/.cpp # advance Sprite frame from AnimationState
│   │
│   ├── input/
│   │   └── InputState.hpp/.cpp     # keyboard/mouse snapshot; World resource
│   │
│   ├── scene/
│   │   └── Scene.hpp/.cpp          # named entity group stub
│   │
│   └── engine/
│       └── Engine.hpp/.cpp         # main loop + system dispatch + SDL init
│
├── example/
│   ├── CMakeLists.txt
│   └── main.cpp
│
├── assets/
│   └── shaders/
│       ├── src/                    # GLSL source
│       └── compiled/               # SPIR-V .spv blobs (glslc output)
│
└── tests/
    ├── CMakeLists.txt
    ├── test_ecs.cpp
    └── test_query.cpp
```

---

## ECS Architecture

### Entity IDs

```cpp
struct EntityId {
    uint32_t index;       // slot in entity_records_ array
    uint32_t generation;  // incremented on destroy; detects stale handles
};
```

`NULL_ENTITY` is the invalid sentinel `{INVALID_INDEX, INVALID_GEN}`.

### Component Type IDs

Each component type gets a unique integer ID via a static template counter:

```cpp
template<typename T>
static ComponentTypeId ComponentRegistry::get() {
    static const ComponentTypeId id = next_id(); // C++11 magic static, thread-safe
    return id;
}
```

No RTTI required. IDs are not stable across processes; never serialize raw IDs.

### Archetype

An `Archetype` is a sorted, canonical set of `ComponentTypeId`s. `{A,B}` and
`{B,A}` produce the same archetype. `Archetype::Hash` enables use as an
`unordered_map` key.

Key methods:
- `has(ComponentTypeId)` — O(log n) binary search
- `with(ComponentTypeId)` — returns new archetype (sorted insertion)
- `without(ComponentTypeId)` — returns new archetype (sorted removal)

### ArchetypeStorage (SoA Table)

One storage per unique archetype. Each storage holds one `ComponentPool` per
component type. Rows are parallel: row N across all pools is the same entity.

```
ArchetypeStorage: [Transform, Sprite]
  Pool<Transform>: [(10,20,0,1,1), (30,40,0,1,1), ...]
  Pool<Sprite>:    [tex1, tex2, ...]
  entities_:       [EntityId{0,1}, EntityId{1,1}, ...]
```

**`ComponentPool::swap_remove(row)`** is the O(1) delete: moves the last element
into the vacated slot, then pops. The `World` must immediately update the
displaced entity's row in `entity_records_`. This is the most critical invariant.

All components **must** have `noexcept` move constructors (enforced by
`static_assert` in `World::add_component`).

### World

The central hub. Owns all `ArchetypeStorage` instances and a flat `entity_records_`
array for O(1) entity lookup.

Component mutation triggers **archetype migration**:
1. Compute new archetype (old ± component type)
2. Get/create the target storage
3. `migrate_from`: move all existing components to new storage row
4. Place the new/removed component
5. `swap_remove` from old storage; update displaced entity's row
6. Update migrated entity's `EntityRecord`

**Structural mutations during query iteration are forbidden** (debug assert via
`iterating_` flag).

**Resources** are global, non-entity data (e.g., `InputState`). Stored via
`world.add_resource<T>()` / `world.get_resource<T>()`. Uses a separate ID counter
from components to avoid collisions.

### Query

```cpp
world.query<Transform, Sprite>().each([](EntityId e, Transform& t, Sprite& s) {
    // ...
});
```

`World::query<C...>()` scans all known archetypes and retains those whose component
set is a superset of `{C...}`. Typically <500 archetypes; the linear scan is fast.

The `each` implementation uses C++20 fold expressions to unpack typed pool pointers
at compile time. The template implementation lives in `Query.inl`, included at the
bottom of `Query.hpp` to break circular dependencies.

---

## Renderer Architecture

### SDL_GPU Command Buffer Lifecycle (per frame)

```
begin_frame()
  └─ SDL_AcquireGPUCommandBuffer → cmd_buf_

Systems call submit_draw(DrawCommand) → enqueued in draw_queue_

end_frame()
  └─ flush_draw_commands()
       ├─ SDL_BeginGPURenderPass
       ├─ [bind pipeline, draw each command]
       ├─ SDL_EndGPURenderPass
       └─ SDL_SubmitGPUCommandBuffer
```

The command buffer is valid only for the thread that acquired it. It must never
be stored across frames.

### Depth Texture

Created at init and recreated on every `on_resize`. Bound as the depth attachment
in `SDL_BeginGPURenderPass`.

### GPUPipelineCache

Maps `PipelineKey { shader_ids, color_format, depth_format, prim_type }` to
`SDL_GPUGraphicsPipeline*`. Pipelines are created on first use and reused
thereafter.

### Shader Loading

SPIR-V blobs are loaded from `assets/shaders/compiled/*.spv` at runtime via
`ShaderLoader`. Compile GLSL to SPIR-V offline with:
```bash
glslc assets/shaders/src/sprite.vert.glsl -o assets/shaders/compiled/sprite.vert.spv
glslc assets/shaders/src/sprite.frag.glsl -o assets/shaders/compiled/sprite.frag.spv
```

### NDC / Coordinate System

SDL_GPU uses Vulkan/Metal NDC (Y down, depth [0,1]). Configure GLM with:
```cpp
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
```
Use `glm::perspectiveRH_ZO` for 3D projection and `glm::orthoRH_ZO` for 2D.

---

## Game Loop

Fixed timestep accumulator at 60 Hz for deterministic physics. Uses
`SDL_GetTicksNS()` for nanosecond precision.

```
prev = SDL_GetTicksNS()
accumulator = 0
fixed_ns = 1_000_000_000 / 60

loop:
  now = SDL_GetTicksNS()
  dt = min(now - prev, 100ms in ns)   // cap: prevent spiral of death
  prev = now
  accumulator += dt

  while accumulator >= fixed_ns:
    InputSystem.update(world, 1/60)
    PhysicsSystem2D.update(world, 1/60)
    AnimationSystem.update(world, 1/60)
    accumulator -= fixed_ns

  render_dt = dt / 1e9f
  TransformSystem.update(world, render_dt)
  renderer.begin_frame()
  RenderSystem2D.update(world, render_dt)
  RenderSystem3D.update(world, render_dt)
  renderer.end_frame()
```

### System Registration

```cpp
engine.add_system<InputSystem>(SystemSchedule::FIXED, /*quit_flag*/);
engine.add_system<PhysicsSystem2D>(SystemSchedule::FIXED);
engine.add_system<AnimationSystem>(SystemSchedule::FIXED);
engine.add_system<TransformSystem>(SystemSchedule::VARIABLE);
engine.add_system<RenderSystem2D>(SystemSchedule::VARIABLE, renderer_2d);
engine.add_system<RenderSystem3D>(SystemSchedule::VARIABLE, renderer_3d);
```

---

## InputState

Stored as a World resource. Snapshot updated by `InputSystem` each fixed tick:
- `clear_frame_state()` — zeroes `keys_pressed`, `keys_released`, `mouse_*_pressed/released`
- Processes all pending SDL events, updating state arrays

Usage from other systems:
```cpp
auto& input = world.get_resource<InputState>();
if (input.key_down(SDL_SCANCODE_SPACE)) { /* jump */ }
```

---

## Known Gotchas

| # | Issue | Mitigation |
|---|---|---|
| 1 | `swap_remove` cascade | Immediately update displaced entity's `EntityRecord::row` |
| 2 | SDL_GPU + SDL_Renderer window conflict | SDL_GPU only — no SDL_Renderer anywhere |
| 3 | Depth texture resize | Destroy + recreate `depth_tex_` in `on_resize` |
| 4 | GLM NDC for Vulkan | `GLM_FORCE_DEPTH_ZERO_TO_ONE`, use `*RH_ZO` variants |
| 5 | Mutation inside `each` | Forbidden; debug-mode `iterating_` assert |
| 6 | Transform hierarchy cycles | Debug-mode visited-set DFS guard |
| 7 | Resource vs component ID collision | Separate `ResourceRegistry` counter |
| 8 | noexcept move requirement | `static_assert` in `World::add_component<T>` |

---

## Build

```bash
# Prerequisites: VCPKG_ROOT set, Ninja installed
cmake --preset default    # configures with vcpkg toolchain
cmake --build build       # builds HiromiEngine + hiromi_example

# Run example
./build/example/hiromi_example
```

Dependencies installed by vcpkg: `sdl3`, `sdl3-image` (png+jpeg), `sdl3-ttf`,
`glm`, `fmt`.
