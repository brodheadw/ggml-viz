# examples/llama_demo/CMakeLists.txt

# Build llama.cpp first
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/llama.cpp 
                 ${CMAKE_BINARY_DIR}/third_party/llama.cpp 
                 EXCLUDE_FROM_ALL)

# Create llama demo executable
add_executable(run_llama_vis
    run_llama_vis.cpp
)

# Include directories
target_include_directories(run_llama_vis PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/llama.cpp/include
    ${CMAKE_SOURCE_DIR}/third_party/llama.cpp/src
    ${CMAKE_SOURCE_DIR}/third_party/llama.cpp/common
)

# Link required libraries
target_link_libraries(run_llama_vis PRIVATE
    # Our ggml-viz libraries
    ggml_hook
    ggml_utils
    ggml_frontend
    
    # llama.cpp libraries
    llama
    common
    
    # System libraries
    Threads::Threads
)

# Set output directory
set_target_properties(run_llama_vis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
)