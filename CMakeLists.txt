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
    
    # Check what target MinHook actually creates and create alias
    if(TARGET minhook)
        add_library(MinHook ALIAS minhook)
        set_target_properties(minhook PROPERTIES OUTPUT_NAME "MinHook")
    elseif(TARGET MinHook)
        # Target is already called MinHook, no alias needed
        set_target_properties(MinHook PROPERTIES OUTPUT_NAME "MinHook")
    elseif(TARGET libminhook)
        add_library(MinHook ALIAS libminhook)
        set_target_properties(libminhook PROPERTIES OUTPUT_NAME "MinHook")
    else()
        message(STATUS "Available targets after MinHook FetchContent:")
        get_property(_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
        foreach(_target ${_targets})
            message(STATUS "  - ${_target}")
        endforeach()
        message(FATAL_ERROR "MinHook target not found after FetchContent_MakeAvailable")
    endif()
    
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

