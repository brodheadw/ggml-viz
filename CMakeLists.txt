cmake_minimum_required(VERSION 3.15)
project(ggml-viz VERSION 0.1 LANGUAGES C CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enforce C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# Build GUI dependencies first
add_subdirectory(third_party)

# Build our instrumentation library
add_subdirectory(src)

# Enable visualization hooks in GGML
set(GGML_VIZ_ENABLE_HOOKS ON CACHE BOOL "Enable hooks for ggml-viz visualization" FORCE)

# Bring in ggml submodule (will define a 'ggml' target)
add_subdirectory(third_party/ggml EXCLUDE_FROM_ALL)

# Build and run tests
enable_testing()
add_subdirectory(tests)


# Minimal test with real GGML computation (no hook library to avoid conflicts)
add_executable(test_minimal_hooks test_minimal_hooks.c)
target_link_libraries(test_minimal_hooks PRIVATE 
    ggml
    ggml-cpu
    ggml-base
)
target_include_directories(test_minimal_hooks PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_definitions(test_minimal_hooks PRIVATE GGML_VIZ_ENABLE_HOOKS)

