# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Add GGML Hook library
add_library(ggml_hook STATIC
    ${CMAKE_SOURCE_DIR}/src/instrumentation/ggml_hook.cpp
    ${CMAKE_SOURCE_DIR}/src/instrumentation/ggml_hook.hpp
)

target_include_directories(ggml_hook PUBLIC
    ${CMAKE_SOURCE_DIR}/src/instrumentation
    ${CMAKE_SOURCE_DIR}/third_party/ggml/include
    ${CMAKE_SOURCE_DIR}/third_party/ggml/src  # For ggml-impl.h
)

target_compile_features(ggml_hook PUBLIC cxx_std_17)
target_compile_definitions(ggml_hook PUBLIC GGML_VIZ_ENABLE_HOOKS)

# Find or build GGML
if(NOT TARGET ggml)
    # Option 1: Use system GGML
    find_package(ggml QUIET)
    
    if(NOT ggml_FOUND)
        # Option 2: Build from submodule
        if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/ggml/CMakeLists.txt)
            add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/ggml build_ggml)
        else()
            message(FATAL_ERROR "GGML not found. Please install or add as submodule.")
        endif()
    endif()
endif()

# Test executable
add_executable(test_ggml_hook
    test_ggml_hook.cpp
)

target_link_libraries(test_ggml_hook PRIVATE
    ggml_hook
    ggml
    ${CMAKE_THREAD_LIBS_INIT}
)

# Copy test to binary dir
add_custom_command(TARGET test_ggml_hook POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test_ggml_hook> ${CMAKE_BINARY_DIR}/bin/
)

# Add test
enable_testing()
add_test(NAME ggml_hook_basic COMMAND test_ggml_hook)

# Additional test for trace file parsing
add_executable(test_trace_reader
    test_trace_reader.cpp
)

target_link_libraries(test_trace_reader PRIVATE
    ggml_hook
)

add_test(NAME trace_reader COMMAND test_trace_reader)