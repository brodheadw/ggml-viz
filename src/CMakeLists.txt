# src/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Add IPC library for cross-platform shared memory
if(NOT TARGET ggml_ipc_core)
    add_library(ggml_ipc_core STATIC
        ipc/ipc_common.hpp
    )
    
    # Add platform-specific IPC implementations
    if(WIN32)
        target_sources(ggml_ipc_core PRIVATE ipc/shm_windows.cpp)
    else()
        target_sources(ggml_ipc_core PRIVATE ipc/shm_posix.cpp)
    endif()

    target_include_directories(ggml_ipc_core
        PUBLIC
          ${CMAKE_CURRENT_SOURCE_DIR}/ipc
    )

    target_link_libraries(ggml_ipc_core
        PUBLIC Threads::Threads
    )

    # Links directory to minhook and psapi targets from FetchContent
    if(WIN32)
        target_link_libraries(ggml_ipc_core PUBLIC MinHook psapi)
    endif()
endif()

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
    utils/config.hpp
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
      ggml_ipc_core
      nlohmann_json::nlohmann_json
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
      glfw
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
)

# Add platform-specific interposition sources
if(WIN32)
    target_sources(ggml_viz_hook PRIVATE instrumentation/win32_interpose.cpp)
    target_compile_definitions(ggml_viz_hook PRIVATE USE_MINHOOK)
elseif(APPLE)
    target_sources(ggml_viz_hook PRIVATE 
        instrumentation/dyld_interpose_simple.cpp
        instrumentation/sched_interpose.mm
    )
# Linux uses the interposition functions directly from ggml_hook.cpp
# No additional files needed - LD_PRELOAD works with the existing function definitions
endif()

target_include_directories(ggml_viz_hook
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/instrumentation
      ${CMAKE_CURRENT_SOURCE_DIR}
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

# Add config and logger sources directly to ggml_viz_hook to avoid circular dependency
target_sources(ggml_viz_hook PRIVATE 
    utils/config.cpp
    utils/logger.cpp
)

target_link_libraries(ggml_viz_hook
    PUBLIC 
      ggml_ipc_core
      nlohmann_json::nlohmann_json
      Threads::Threads
      # Note: We intentionally don't link ggml-base to allow function interception
)

# Links directory to minhook and psapi targets from FetchContent
if(WIN32)
    target_link_libraries(ggml_viz_hook PRIVATE MinHook psapi)
endif()

# Platform-specific link options
if(APPLE)
    # Allow unresolved symbols for scheduler interposition
    target_link_options(ggml_viz_hook PRIVATE
        "-undefined" "dynamic_lookup")
elseif(UNIX)
    # Linux-specific options for symbol interposition
    target_link_options(ggml_viz_hook PRIVATE
        "-Wl,--allow-shlib-undefined")
endif()

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
elseif(WIN32)
    set_target_properties(ggml_viz_hook PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
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
      ggml  # Core GGML library for ggml_abort and other functions
)

# Add Windows-specific libraries
if(WIN32)
    target_link_libraries(ggml-viz PRIVATE ws2_32)
endif()