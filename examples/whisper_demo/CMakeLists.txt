# examples/whisper_demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Install the real demo script
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run_whisper_demo.sh
    ${CMAKE_BINARY_DIR}/examples/whisper_demo/run_whisper_demo.sh
    COPYONLY
)

# Make the script executable
if(UNIX)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run_whisper_demo.sh
         DESTINATION ${CMAKE_BINARY_DIR}/examples/whisper_demo/
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                         GROUP_READ GROUP_EXECUTE
                         WORLD_READ WORLD_EXECUTE)
endif()

# Copy config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/whisper_demo_config.json
    ${CMAKE_BINARY_DIR}/examples/whisper_demo/whisper_demo_config.json
    COPYONLY
)