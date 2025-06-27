cmake_minimum_required(VERSION 3.15)
project(ggml_viz_tests)

# test_ggml_hook
add_executable(test_ggml_hook
    test_ggml_hook.cpp
)

target_link_libraries(test_ggml_hook
    PRIVATE
      ggml_hook        # from src/CMakeLists.txt
      ggml             # the ggml submodule library
      Threads::Threads
)

add_test(NAME ggml_hook_basic
         COMMAND test_ggml_hook)