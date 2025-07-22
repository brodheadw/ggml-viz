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
- **Linux (x64)**: LD_PRELOAD with symbol interposition
  - POSIX shared memory (`shm_open`/`mmap`)
  - Dynamic library loading via `dlsym(RTLD_NEXT)`
  - Socket API with MSG_NOSIGNAL support

- **macOS (arm64/x64)**: DYLD_INSERT_LIBRARIES with dynamic lookup  
  - POSIX shared memory (`shm_open`/`mmap`)
  - DYLD interposition macros for guaranteed symbol replacement
  - F_FULLFSYNC for immediate disk flushing in live mode

- **Windows 10+ (x64)**: MinHook API hooking with DLL injection
  - Windows file mapping (`CreateFileMappingW`/`MapViewOfFile`)
  - MinHook runtime patching for `ggml_backend_sched_graph_compute`
  - Winsock2 API with proper type casting and error handling
  - Automatic DLL initialization via `DllMain`

### Build System
- **Cross-platform CMake** with platform-specific source selection
- **Zero external dependencies** on Windows (MinHook built from source)
- **GitHub Actions CI/CD** testing all platforms automatically
- **Visual Studio multi-config** support with correct executable paths

See README.md for detailed platform-specific implementation details and build instructions.