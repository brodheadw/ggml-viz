# examples/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Install the main demo runner script
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run_demos.sh
    ${CMAKE_BINARY_DIR}/examples/run_demos.sh
    COPYONLY
)

# Make the script executable
if(UNIX)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run_demos.sh
         DESTINATION ${CMAKE_BINARY_DIR}/examples/
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                         GROUP_READ GROUP_EXECUTE
                         WORLD_READ WORLD_EXECUTE)
endif()

# Add LLaMA demo
add_subdirectory(llama_demo)

# Add Whisper demo (if enabled)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/whisper_demo/CMakeLists.txt)
    add_subdirectory(whisper_demo)
endif()