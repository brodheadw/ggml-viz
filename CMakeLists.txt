cmake_minimum_required(VERSION 3.15)
project(ggml-viz VERSION 0.1 LANGUAGES C CXX OBJC)
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


# Test executables are now defined in tests/CMakeLists.txt

