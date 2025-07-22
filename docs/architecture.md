# Architecture Documentation

This document describes the architecture of ggml-viz.

## Overview

GGML Visualizer is a cross-platform real-time dashboard for visualizing GGML-based LLM runtimes. It uses a multi-component architecture with instrumentation hooks, IPC communication, and a frontend visualization system.

## Components

### 1. Instrumentation Layer
- Platform-specific hooks for capturing GGML events
- Cross-platform event recording with minimal overhead

### 2. IPC Layer  
- Shared memory communication between processes
- Lock-free ring buffer for high-throughput event streaming

### 3. Frontend
- ImGui-based desktop interface
- Real-time visualization of compute graphs and performance metrics

## Platform Support

### Production Ready ✅
- **Linux (x64)**: LD_PRELOAD with conditional compilation architecture
  - POSIX shared memory (`shm_open`/`mmap`)
  - Conditional interception functions via `GGML_VIZ_SHARED_BUILD` preprocessor flag
  - Static library contains data structures, shared library contains interception functions
  - Dynamic library loading via `dlsym(RTLD_NEXT)` for original function lookup
  - X11 GLFW backend (Wayland disabled for broader compatibility)
  - Socket API with platform-specific constants (MSG_NOSIGNAL handling)

- **macOS (arm64/x64)**: DYLD_INSERT_LIBRARIES with dynamic lookup  
  - POSIX shared memory (`shm_open`/`mmap`)
  - DYLD interposition macros for guaranteed symbol replacement
  - F_FULLFSYNC for immediate disk flushing in live mode

- **Windows 10+ (x64)**: MinHook API hooking with DLL injection
  - Windows file mapping (`CreateFileMappingW`/`MapViewOfFile`)
  - MinHook runtime patching for `ggml_backend_sched_graph_compute`
  - Winsock2 API with proper type casting and error handling
  - Automatic DLL initialization via `DllMain`

### Build System Architecture
- **Cross-platform CMake** with platform-specific source selection and conditional compilation
- **Robust dependency management**: 
  - Windows: MinHook built from source (zero external dependencies)
  - Linux: X11 fallback for GLFW (avoids Wayland scanner dependency)
  - macOS: Accelerate framework integration with Metal backend support
- **Symbol collision resolution** via build-target-specific compilation flags
- **GitHub Actions CI/CD** with comprehensive cross-platform testing
- **Multi-config build support**: Visual Studio (Windows), Makefiles (Linux/macOS)

See README.md for detailed platform-specific implementation details and build instructions.