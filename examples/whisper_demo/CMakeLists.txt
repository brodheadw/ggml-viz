# examples/whisper_demo/CMakeLists.txt

# Create whisper demo executable (placeholder implementation)
add_executable(run_whisper_vis
    run_whisper_vis.cpp
)

# Include directories
target_include_directories(run_whisper_vis PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link required libraries
target_link_libraries(run_whisper_vis PRIVATE
    # Our ggml-viz libraries
    ggml_hook
    ggml_utils
    ggml_frontend
    
    # System libraries
    Threads::Threads
)

# Set output directory
set_target_properties(run_whisper_vis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
)