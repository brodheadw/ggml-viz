# Architecture Documentation

This document describes the architecture of ggml-viz.

## Overview

GGML Visualizer is a cross-platform real-time dashboard for visualizing GGML-based LLM runtimes. It uses a multi-component architecture with instrumentation hooks, IPC communication, and a frontend visualization system.

## Components

### 1. Configuration Management System
- **ConfigManager**: Thread-safe singleton for centralized configuration access
- **JSON Configuration Files**: Structured settings with schema validation using nlohmann/json
- **Configuration Precedence**: CLI flags > config files > environment variables > defaults
- **Hot-reload Capability**: Runtime configuration updates with atomic operations
- **Integration**: All components use ConfigManager instead of direct environment variable access

### 2. Instrumentation Layer
- Platform-specific hooks for capturing GGML events
- Cross-platform event recording with minimal overhead
- Configuration-driven instrumentation settings (max events, timing, memory tracking)

### 3. IPC Layer  
- Shared memory communication between processes
- Lock-free ring buffer for high-throughput event streaming
- Configurable buffer sizes and polling intervals

### 4. Frontend
- ImGui-based desktop interface
- Real-time visualization of compute graphs and performance metrics
- Configuration-driven UI settings and visualization options

### 5. Demo Applications
- **LLaMA Demo**: Transformer architecture simulation with realistic attention mechanisms
- **Whisper Demo**: Audio processing pipeline with encoder-decoder architecture
- **Config-driven Setup**: Both demos use JSON configuration files for reproducible setups

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
- **Demo Integration**: Examples directory included in main build with proper target dependencies

## Configuration Flow Architecture

The configuration system implements a hierarchical flow with proper precedence handling:

```
CLI Arguments → ConfigManager.load_with_precedence() → JSON Config File → Environment Variables → Built-in Defaults
      ↓
ConfigManager (Singleton)
      ↓
┌─────────────┬─────────────┬─────────────┬─────────────┐
│   GGMLHook  │   Logger    │    CLI      │    IPC      │
│ (deprecated │ (integrated)│ (integrated)│  (planned)  │
│  configure) │             │             │             │
└─────────────┴─────────────┴─────────────┴─────────────┘
```

### Configuration Integration Patterns

**Thread-Safe Access**: All components use atomic shared_ptr operations to access configuration
**Precedence Enforcement**: ConfigManager handles precedence automatically during load_with_precedence()
**Type Safety**: JSON schema validation with detailed error reporting
**Backward Compatibility**: Environment variables still work but are deprecated in favor of ConfigManager

See README.md for detailed platform-specific implementation details and build instructions.