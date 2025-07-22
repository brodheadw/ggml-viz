cmake_minimum_required(VERSION 3.15)
project(ggml_viz_tests)

# Create a test-specific version of ggml_hook without function overrides
add_library(ggml_hook_test STATIC
    ${CMAKE_SOURCE_DIR}/src/instrumentation/ggml_hook.cpp
    ${CMAKE_SOURCE_DIR}/src/instrumentation/ggml_hook.hpp
)

target_include_directories(ggml_hook_test
    PUBLIC
      ${CMAKE_SOURCE_DIR}/src/instrumentation
      ${CMAKE_SOURCE_DIR}/third_party/ggml/include
      ${CMAKE_SOURCE_DIR}/third_party/ggml/src        # for ggml-impl.h
)

target_compile_definitions(ggml_hook_test
    PUBLIC 
      GGML_VIZ_ENABLE_HOOKS
      GGML_VIZ_TEST_MODE  # This prevents function override compilation
)

target_link_libraries(ggml_hook_test
    PUBLIC Threads::Threads
)

# test_ggml_hook
add_executable(test_ggml_hook
    test_ggml_hook.cpp
)

target_link_libraries(test_ggml_hook
    PRIVATE
      ggml_hook_test   # Use test-specific version without function overrides
      ggml             # the ggml submodule library
      Threads::Threads
)

add_test(NAME ggml_hook_basic
         COMMAND test_ggml_hook)

# test_trace_reader  
add_executable(test_trace_reader
    test_trace_reader.cpp
)

target_link_libraries(test_trace_reader
    PRIVATE
      ggml_utils       # from src/CMakeLists.txt (includes ggml_hook)
      ggml             # the ggml submodule library
      Threads::Threads
)

add_test(NAME trace_reader_basic
         COMMAND test_trace_reader ${CMAKE_CURRENT_SOURCE_DIR}/assets/test_trace.ggmlviz)

# test_logger
add_executable(test_logger
    test_logger.cpp
)

target_link_libraries(test_logger
    PRIVATE
      ggml_utils       # from src/CMakeLists.txt (includes logger)
      Threads::Threads
)

add_test(NAME logger_basic
         COMMAND test_logger)