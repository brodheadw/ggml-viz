# Third-Party Dependencies and Licenses

This document lists all third-party dependencies used in GGML Visualizer and their respective licenses.

## Core Dependencies

### GGML
- **Repository**: https://github.com/ggerganov/ggml
- **License**: MIT License
- **Usage**: Core machine learning graph library and compute backend
- **Location**: `third_party/ggml/` (git submodule)

### Dear ImGui
- **Repository**: https://github.com/ocornut/imgui
- **License**: MIT License
- **Usage**: Desktop GUI framework for visualization interface
- **Location**: `third_party/imgui/`

### GLFW
- **Repository**: https://github.com/glfw/glfw
- **License**: zlib/libpng License
- **Usage**: Cross-platform window and input handling
- **Location**: `third_party/glfw/`

## Build Dependencies

### CMake
- **Website**: https://cmake.org/
- **License**: BSD 3-Clause License
- **Usage**: Build system and dependency management
- **Minimum Version**: 3.15

## Platform-Specific Dependencies

### macOS
- **Accelerate Framework** - Apple's optimized linear algebra library
- **Metal Framework** - Apple's GPU compute framework (disabled by default)
- **Cocoa** - Native window management

### Linux
- **OpenGL** - Graphics rendering
- **X11 libraries** - Window management
- **pthreads** - Threading support

### Windows
- **OpenGL** - Graphics rendering
- **Win32 API** - Window management
- **MSVC Runtime** - C++ standard library

## Development Dependencies

### Optional Tools
- **ccache** - Compiler cache for faster builds
- **OpenMP** - Parallel processing (not required)
- **pkg-config** - Library discovery

## License Compatibility

All dependencies are compatible with Apache 2.0 license:
- MIT License:  Compatible
- BSD 3-Clause:  Compatible  
- zlib/libpng:  Compatible

## Attribution Requirements

### ImGui
This software uses Dear ImGui, which is licensed under the MIT License.
Copyright (c) 2014-2024 Omar Cornut

### GLFW
This software uses GLFW, which is licensed under the zlib/libpng License.
Copyright (c) 2002-2006 Marcus Geelnard
Copyright (c) 2006-2019 Camilla Löwy

### GGML
This software uses GGML, which is licensed under the MIT License.
Copyright (c) 2022-2024 Georgi Gerganov

---

## Notes

- No icons are currently used (previous claims about CC-BY-4.0 icons were incorrect)
- All third-party code is included via git submodules or system dependencies
- No proprietary or restrictive licenses are used

**Last Updated**: 2025-07-01