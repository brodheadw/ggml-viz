# examples/llama_demo/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Install the real demo scripts (both bash and PowerShell)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run_llama_demo.sh
    ${CMAKE_BINARY_DIR}/examples/llama_demo/run_llama_demo.sh
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run_llama_demo.ps1
    ${CMAKE_BINARY_DIR}/examples/llama_demo/run_llama_demo.ps1
    COPYONLY
)

# Make the bash script executable on Unix
if(UNIX)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run_llama_demo.sh
         DESTINATION ${CMAKE_BINARY_DIR}/examples/llama_demo/
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                         GROUP_READ GROUP_EXECUTE
                         WORLD_READ WORLD_EXECUTE)
endif()

# Copy config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/llama_demo_config.json
    ${CMAKE_BINARY_DIR}/examples/llama_demo/llama_demo_config.json
    COPYONLY
)