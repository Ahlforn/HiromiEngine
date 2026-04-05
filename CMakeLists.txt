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

# ---- Subdirectories ------------------------------------------------------------
add_subdirectory(engine)
add_subdirectory(example)

option(HIROMI_BUILD_TESTS "Build HiromiEngine tests" OFF)
if(HIROMI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
