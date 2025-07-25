cmake_minimum_required(VERSION 3.15)

# Third-party dependencies for ggml-viz

# GLFW - Windowing library
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# ImGui - Immediate mode GUI library
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    # Platform backends
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    imgui/
    imgui/backends/
)

target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES})

if(APPLE)
    target_compile_definitions(imgui PUBLIC GL_SILENCE_DEPRECATION)
endif()

# Create OpenGL loader files for cross-platform compatibility
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/opengl_loader.h" "#pragma once\n#include <glad/glad.h>\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/imgui_opengl_loader.h" "#pragma once\n#include \"opengl_loader.h\"\n")

if(APPLE)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/opengl_loader_macos.cpp" 
         "#include \"opengl_loader.h\"\n// OpenGL loader implementation for macOS\n")
endif()

# Make headers available to parent project
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")