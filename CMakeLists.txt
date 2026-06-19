cmake_minimum_required(VERSION 3.22)
project(AvellanedaStoikov VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit a compile_commands.json for clangd / IDE tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Warnings ──────────────────────────────────────────────────────────────────
add_compile_options(
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Enable aggressive optimization for Release builds
if(NOT MSVC)
    add_compile_options($<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-march=native>)
else()
    add_compile_options($<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/arch:AVX2>)
endif()

# ── Dependencies ──────────────────────────────────────────────────────────────
include(FetchContent)

# nlohmann/json  (header-only, no tests, no install)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install    OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# Catch2 v3 for testing framework
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

# Melissa O'Neill's PCG-cpp (header-only, fast, statistically superior RNG)
FetchContent_Declare(pcg_cpp
    GIT_REPOSITORY https://github.com/imneme/pcg-cpp.git
    GIT_TAG        v0.98.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(pcg_cpp)

# ── Source Auto-Detection ─────────────────────────────────────────────────────
# This logic checks if Phase 2 source files exist yet. This allows the same
# build system to seamlessly compile Phase 1 first, then self-upgrade as you
# drop source files into src/ without breaking intermediate compilations.
set(CORE_SOURCES)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/Simulator.cpp")
    list(APPEND CORE_SOURCES src/core/Simulator.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/Agent.cpp")
    list(APPEND CORE_SOURCES src/core/Agent.cpp)
endif()

# ── Simulator Core Library ────────────────────────────────────────────────────
if(CORE_SOURCES)
    # Phase 2+: Source files detected; compile static library
    add_library(sim_core STATIC ${CORE_SOURCES})
    target_include_directories(sim_core PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${pcg_cpp_SOURCE_DIR}/include
    )
    target_link_libraries(sim_core PUBLIC nlohmann_json::nlohmann_json)
else()
    # Phase 1: Header-only fall-back to allow compilation of Phase 1 files & tests
    add_library(sim_core INTERFACE)
    target_include_directories(sim_core INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${pcg_cpp_SOURCE_DIR}/include
    )
    target_link_libraries(sim_core INTERFACE nlohmann_json::nlohmann_json)
endif()

# ── Main Executable ───────────────────────────────────────────────────────────
add_executable(simulator
    src/main.cpp
)

target_link_libraries(simulator PRIVATE sim_core)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
