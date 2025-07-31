# examples/whisper_demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Create whisper demo executable using pure GGML
add_executable(run_whisper_vis
    run_whisper_vis.cpp
)

# Include directories
target_include_directories(run_whisper_vis PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/ggml/include
    ${CMAKE_SOURCE_DIR}/third_party/ggml/src
)

# Link required libraries
target_link_libraries(run_whisper_vis PRIVATE
    # Our ggml-viz libraries
    ggml_hook
    ggml_utils
    
    # Pure GGML library
    ggml
    
    # System libraries
    Threads::Threads
)

# Set output directory
set_target_properties(run_whisper_vis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
)