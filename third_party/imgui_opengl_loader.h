// third_party/imgui_opengl_loader.h
// Custom OpenGL loader header for ImGui

#pragma once

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>

// Function loader stub for ImGui - no-op on macOS since functions are directly linked
static inline bool ImGui_ImplOpenGL3_LoadGLLoader() { return true; }

#else
// For other platforms, use the default ImGui loader
#include "imgui_impl_opengl3_loader.h"
#endif