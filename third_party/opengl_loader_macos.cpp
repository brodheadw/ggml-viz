// third_party/opengl_loader_macos.cpp
// Custom OpenGL loader for macOS to avoid GLES dependencies

#ifdef __APPLE__

// Include macOS OpenGL headers
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>

// ImGui expects these loader functions when IMGUI_IMPL_OPENGL_LOADER_CUSTOM is defined
extern "C" {

// Stub loader functions - on macOS OpenGL functions are directly available
int ImGui_ImplOpenGL3_Init() {
    return 1; // Success
}

void ImGui_ImplOpenGL3_Shutdown() {
    // Nothing to do on macOS
}

} // extern "C"

#endif // __APPLE__