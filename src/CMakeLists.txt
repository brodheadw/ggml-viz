# src/CMakeLists.txt
project(ggml_viz_instrumentation)

# Add IPC library
add_library(ggml_ipc STATIC
    ipc/ipc_common.hpp
    ipc/shm_posix.cpp
)

target_include_directories(ggml_ipc
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/ipc
)

target_link_libraries(ggml_ipc
    PUBLIC Threads::Threads
)

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
    PUBLIC 
      Threads::Threads
      ggml_ipc
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

# Shared library for automatic injection
add_library(ggml_viz_hook SHARED
    instrumentation/ggml_hook.cpp
    instrumentation/ggml_hook.hpp
    instrumentation/ggml_viz_init.cpp
    instrumentation/dyld_interpose_simple.cpp
    instrumentation/sched_interpose.mm
    instrumentation/linux_interpose.cpp
    ipc/shm_posix.cpp
    utils/logger.cpp
)

target_include_directories(ggml_viz_hook
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation
      ${CMAKE_CURRENT_SOURCE_DIR}/ipc
      ${CMAKE_CURRENT_SOURCE_DIR}                      # for utils/logger.hpp
      ${CMAKE_SOURCE_DIR}/third_party/ggml/include
      ${CMAKE_SOURCE_DIR}/third_party/ggml/src        # for ggml-impl.h
)

target_compile_definitions(ggml_viz_hook
    PUBLIC 
      GGML_VIZ_ENABLE_HOOKS
      GGML_VIZ_SHARED_BUILD
    PRIVATE
      BUILDING_GGML_VIZ
)

target_link_libraries(ggml_viz_hook
    PUBLIC 
      Threads::Threads
      # Note: We intentionally don't link ggml-base to allow function interception
)

# Allow unresolved symbols for scheduler interposition
target_link_options(ggml_viz_hook PRIVATE
    "-undefined" "dynamic_lookup")

# Set shared library properties
# Hide all symbols by default, only export what we explicitly mark
set_target_properties(ggml_viz_hook PROPERTIES
    VERSION 0.1
    SOVERSION 1
    OUTPUT_NAME "ggml_viz_hook"
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# Platform-specific shared library setup
if(APPLE)
    set_target_properties(ggml_viz_hook PROPERTIES
        MACOSX_RPATH ON
        INSTALL_RPATH "@rpath"
        BUILD_WITH_INSTALL_RPATH ON
    )
elseif(UNIX)
    set_target_properties(ggml_viz_hook PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH ON
    )
endif()

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