add_library(HiromiEngine STATIC
    # ECS
    ecs/ArchetypeStorage.cpp
    ecs/World.cpp
    # Renderer
    renderer/Renderer2D.cpp
    renderer/Renderer3D.cpp
    renderer/GPUPipelineCache.cpp
    renderer/ShaderLoader.cpp
    # Input
    input/InputState.cpp
    # Systems
    systems/InputSystem.cpp
    systems/TransformSystem.cpp
    systems/RenderSystem2D.cpp
    systems/RenderSystem3D.cpp
    systems/PhysicsSystem2D.cpp
    systems/AnimationSystem.cpp
    # Scene / Engine
    scene/Scene.cpp
    engine/Engine.cpp
)

target_include_directories(HiromiEngine
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HiromiEngine
    PUBLIC
        SDL3::SDL3
        $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
        SDL3_ttf::SDL3_ttf
        glm::glm
        fmt::fmt
)

target_compile_options(HiromiEngine PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

target_compile_features(HiromiEngine PUBLIC cxx_std_20)

# GLM: Vulkan/Metal NDC (depth [0,1], right-hand)
target_compile_definitions(HiromiEngine PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
)
