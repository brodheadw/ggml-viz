# src/CMakeLists.txt
project(ggml_viz_instrumentation)

add_library(ggml_hook STATIC
    instrumentation/ggml_hook.cpp
    instrumentation/ggml_hook.hpp
)

target_include_directories(ggml_hook
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation
      ${CMAKE_SOURCE_DIR}/third_party/ggml/include
      ${CMAKE_SOURCE_DIR}/third_party/ggml/src        # for ggml-impl.h
)

target_compile_definitions(ggml_hook
    PUBLIC GGML_VIZ_ENABLE_HOOKS
)

target_link_libraries(ggml_hook
    PUBLIC Threads::Threads
)