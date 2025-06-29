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

# Add utils library
add_library(ggml_utils STATIC
    utils/config.cpp
    utils/logger.cpp 
    utils/trace_reader.cpp
    utils/trace_reader.hpp
)

target_include_directories(ggml_utils
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation
      ${CMAKE_SOURCE_DIR}/third_party/ggml/include
      ${CMAKE_SOURCE_DIR}/third_party/ggml/src
)

target_link_libraries(ggml_utils
    PUBLIC 
      ggml_hook
      Threads::Threads
)

# Add frontend library
add_library(ggml_frontend STATIC
    frontend/imgui_app.cpp
    frontend/imgui_widgets.hpp
    frontend/imgui_widgets.cpp
)

target_include_directories(ggml_frontend
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation
      ${CMAKE_CURRENT_SOURCE_DIR}/utils
      ${CMAKE_SOURCE_DIR}/third_party/imgui
      ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
)

target_link_libraries(ggml_frontend
    PUBLIC
      ggml_utils
      imgui
      OpenGL::GL
      Threads::Threads
)

target_compile_definitions(ggml_frontend
    PUBLIC
      IMGUI_IMPL_OPENGL_LOADER_GL3W=0
      IMGUI_DEFINE_MATH_OPERATORS
)

# Main executable
add_executable(ggml-viz
    main.cpp
)

target_include_directories(ggml-viz
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ggml-viz
    PRIVATE
      ggml_frontend
      ggml_utils
      ggml_hook
)