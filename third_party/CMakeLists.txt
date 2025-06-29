# third_party/CMakeLists.txt

# GLFW Configuration
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# ImGui Configuration
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    # OpenGL backend
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/backends/imgui_impl_glfw.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui 
    PUBLIC 
        imgui
        imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
        ${CMAKE_CURRENT_SOURCE_DIR}  # For our custom opengl_loader.h
)

target_link_libraries(imgui 
    PUBLIC 
        glfw
)

# Platform-specific OpenGL linking
if(APPLE)
    target_link_libraries(imgui PUBLIC "-framework OpenGL")
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC OpenGL::GL)
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC OpenGL::GL)
endif()

# Platform-specific OpenGL configuration
if(APPLE)
    target_compile_definitions(imgui 
        PUBLIC 
            GL_SILENCE_DEPRECATION
    )
endif()

# Don't define IMGUI_IMPL_OPENGL_ES2 or IMGUI_IMPL_OPENGL_ES3 - let ImGui use desktop OpenGL