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

- **Linux**: LD_PRELOAD with symbol interposition
- **macOS**: DYLD_INSERT_LIBRARIES with dynamic lookup
- **Windows**: MinHook API hooking (experimental)

See README.md for detailed platform-specific implementation details.