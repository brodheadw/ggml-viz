cmake_minimum_required(VERSION 3.15)
project(ggml-viz VERSION 0.0.7 LANGUAGES C CXX)

# Enable Objective-C only on Apple platforms
if(APPLE)
    enable_language(OBJC)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enforce C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(third_party)
add_subdirectory(third_party/ggml)

# Windows-specific dependencies
if(WIN32)
    include(FetchContent)
    FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG        v1.3.3
    )
    FetchContent_MakeAvailable(minhook)
    
    # Build our own static library from MinHook sources
    add_library(MinHook STATIC
        ${minhook_SOURCE_DIR}/src/buffer.c
        ${minhook_SOURCE_DIR}/src/hook.c
        ${minhook_SOURCE_DIR}/src/trampoline.c
        ${minhook_SOURCE_DIR}/src/hde/hde32.c
        ${minhook_SOURCE_DIR}/src/hde/hde64.c
    )
    target_include_directories(MinHook PUBLIC ${minhook_SOURCE_DIR}/include)
    target_compile_definitions(MinHook PRIVATE MH_STANDALONE)
    set_target_properties(MinHook PROPERTIES
        OUTPUT_NAME "MinHook"
        POSITION_INDEPENDENT_CODE ON
    )

endif()

# Build our instrumentation library
add_subdirectory(src)

# Build examples (demos)
add_subdirectory(examples)

# Build and run tests
enable_testing()
add_subdirectory(tests)


# Test executables are now defined in tests/CMakeLists.txt

