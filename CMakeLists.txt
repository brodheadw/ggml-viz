cmake_minimum_required(VERSION 3.15)
project(ggml-viz VERSION 0.1 LANGUAGES C CXX)

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
    
    # Add Windows-specific compile definitions
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    if(MSVC)
        add_compile_options(/EHsc)
    endif()
endif()

# Build GUI dependencies first
add_subdirectory(third_party)

# Enable visualization hooks in GGML
set(GGML_VIZ_ENABLE_HOOKS ON CACHE BOOL "Enable hooks for ggml-viz visualization" FORCE)

# Bring in ggml submodule (will define a 'ggml' target)
add_subdirectory(third_party/ggml EXCLUDE_FROM_ALL)

# Build our instrumentation library
add_subdirectory(src)

# Build and run tests
enable_testing()
add_subdirectory(tests)


# Test executables are now defined in tests/CMakeLists.txt

