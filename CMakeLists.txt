cmake_minimum_required(VERSION 3.27)
project(HiromiEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure vcpkg (imported) include paths use -I instead of -isystem,
# so they take priority over system-installed headers (e.g. /usr/local/include).
set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)

# ---- Dependencies (provided by vcpkg) ----------------------------------------
find_package(SDL3        CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf    CONFIG REQUIRED)
find_package(glm         CONFIG REQUIRED)
find_package(fmt         CONFIG REQUIRED)

# ---- Shader compilation --------------------------------------------------------
# Compile GLSL → SPIR-V (glslc) → MSL (spirv-cross) at build time.
find_program(GLSLC glslc)
find_program(SPIRV_CROSS spirv-cross)

set(SHADER_SRC_DIR  "${CMAKE_SOURCE_DIR}/assets/shaders/src")
set(SHADER_OUT_DIR  "${CMAKE_SOURCE_DIR}/assets/shaders/compiled")

set(SHADER_SOURCES
    sprite.vert
    sprite.frag
    mesh.vert
    mesh.frag
)

set(SHADER_OUTPUTS "")
foreach(SHADER IN LISTS SHADER_SOURCES)
    # Derive stage from extension (.vert → vertex, .frag → fragment)
    string(REGEX MATCH "\\.[^.]+$" STAGE_EXT "${SHADER}")
    if(STAGE_EXT STREQUAL ".vert")
        set(STAGE "vertex")
    else()
        set(STAGE "fragment")
    endif()

    set(SRC  "${SHADER_SRC_DIR}/${SHADER}.glsl")
    set(SPV  "${SHADER_OUT_DIR}/${SHADER}.spv")
    set(MSL  "${SHADER_OUT_DIR}/${SHADER}.msl")

    if(GLSLC)
        add_custom_command(OUTPUT "${SPV}"
            COMMAND "${GLSLC}" -fshader-stage=${STAGE} "${SRC}" -o "${SPV}"
            DEPENDS "${SRC}"
            COMMENT "GLSL → SPIR-V: ${SHADER}")
    endif()

    if(GLSLC AND SPIRV_CROSS)
        add_custom_command(OUTPUT "${MSL}"
            COMMAND "${SPIRV_CROSS}" --msl "${SPV}" --output "${MSL}"
            DEPENDS "${SPV}"
            COMMENT "SPIR-V → MSL:  ${SHADER}")
        list(APPEND SHADER_OUTPUTS "${SPV}" "${MSL}")
    elseif(GLSLC)
        list(APPEND SHADER_OUTPUTS "${SPV}")
    endif()
endforeach()

if(SHADER_OUTPUTS)
    add_custom_target(compile_shaders ALL DEPENDS ${SHADER_OUTPUTS})
endif()

# ---- Subdirectories ------------------------------------------------------------
add_subdirectory(engine)
add_subdirectory(example)

option(HIROMI_BUILD_TESTS "Build HiromiEngine tests" OFF)
if(HIROMI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
