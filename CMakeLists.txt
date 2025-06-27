cmake_minimum_required(VERSION 3.15)
project(ggml-viz VERSION 0.1 LANGUAGES C CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enforce C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Threads
find_package(Threads REQUIRED)

# Bring in ggml submodule (will define a 'ggml' target)
add_subdirectory(third_party/ggml EXCLUDE_FROM_ALL)

# Build our instrumentation library
add_subdirectory(src)

# Build and run tests
enable_testing()
add_subdirectory(tests)

