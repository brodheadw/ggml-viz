# examples/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Add LLaMA demo
add_subdirectory(llama_demo)

# Add Whisper demo (if enabled)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/whisper_demo/CMakeLists.txt)
    add_subdirectory(whisper_demo)
endif()