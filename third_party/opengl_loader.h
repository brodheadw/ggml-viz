// third_party/opengl_loader.h
// Simple OpenGL loader for macOS
#pragma once

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>

// Provide stub loader functions that ImGui expects
static inline bool ImGui_ImplOpenGL3_Init() { return true; }
static inline void ImGui_ImplOpenGL3_Shutdown() {}

#else
// For other platforms, we would include proper GL loaders here
#include <GL/gl.h>
#endif