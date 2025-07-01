# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Comprehensive User Guide (USER_GUIDE.md) with llama.cpp integration examples
- Updated project documentation with accurate feature status
- Fixed macOS build instructions with Metal backend workaround

### Changed
- Updated README.md with accurate feature status (removed incorrect "Beta" labels)
- Corrected CLI usage documentation to reflect actual environment variable approach
- Updated CLAUDE.md with current build process and known issues

### Fixed
- macOS Metal shader compilation issues (use -DGGML_METAL=OFF as workaround)

## [0.1.0] - 2025-07-01

### Added
- **Core Instrumentation System** (498 LOC) - Complete GGML hook infrastructure
  - Universal backend hooks at `ggml_backend_graph_compute()` level
  - Automatic event capture for any GGML-based application
  - Lock-free ring buffer with atomic operations for event recording
  - Binary trace format with `.ggmlviz` extension and magic header "GGMLVIZ1"
  
- **Full CLI Interface** (220 LOC) - Production-ready command line tool
  - Comprehensive argument parsing with `getopt_long`
  - Support for `--help`, `--version`, `--verbose`, `--live`, `--port`, `--config` options
  - Input validation for port numbers and file existence
  - Clear error messages and warnings for unimplemented features
  - Environment variable documentation and examples

- **Desktop UI** (1,379 LOC total) - ImGui-based visualization interface
  - Main UI application with trace file loading capability (593 LOC)
  - Custom ImGui widgets for graph visualization and inspection (786 LOC)
  - Static compute graph visualization with operation details
  - Graph import support via `ggml_graph_dump_dot` integration

- **Trace System** (457 LOC) - File generation and analysis
  - Binary .ggmlviz file parsing and event replay (134 LOC)
  - Event processing and data collection (324 LOC)
  - Automatic file flushing every 4096 events
  - Cross-platform compatibility

- **Auto-Initialization** (169 LOC) - Environment variable configuration
  - Activation via `GGML_VIZ_OUTPUT` environment variable
  - Configurable verbosity with `GGML_VIZ_VERBOSE`
  - Optional disable with `GGML_VIZ_DISABLE`
  - Advanced configuration variables for event limits and tracking options

- **Testing Infrastructure**
  - Unit tests for core instrumentation (`test_ggml_hook`)
  - Trace reader validation (`test_trace_reader`)
  - CTest integration with CMake build system
  - Test creates 1024x1024 matrix operations to validate hook functionality

- **Cross-Platform Support**
  - macOS (arm64, x86_64) with Accelerate framework integration
  - Linux (x86_64) with CPU backend support
  - Windows compatibility (partial)
  - Raspberry Pi 5 support (NEON optimizations)

### Technical Implementation
- **Performance**: Overhead not yet benchmarked - needs measurement
- **Thread Safety**: Lock-free ring buffer prevents unbounded memory growth
- **Backend Support**: Works with CPU, Metal, CUDA, Vulkan, OpenCL backends
- **Integration**: Zero-modification integration with existing llama.cpp installations

### Build System
- CMake build system with proper dependency management (minimum version 3.15)
- Git submodule integration for GGML and dependencies
- Automatic OpenGL library detection and linking
- Platform-specific optimization flags (NEON, AVX2, etc.)

### Documentation
- Comprehensive README.md with quick start guide
- Detailed architecture documentation
- Integration examples and usage patterns
- Development setup instructions
- Troubleshooting guide for common issues

## [0.0.1] - Initial Development

### Added
- Initial project scaffolding and repository structure
- Basic GGML submodule integration
- Prototype hook system in ggml-cpu.c
- Initial CMake configuration
- Basic test infrastructure

### Development History
- Started with CPU-specific hooks in ggml-cpu.c
- Evolved to universal backend hooks in ggml-backend.cpp
- Migrated from simple printf debugging to structured event capture
- Developed from 25 LOC CLI stub to full 220 LOC interface
- Built comprehensive ImGui frontend from basic proof-of-concept

---

## Version Numbering

This project uses [Semantic Versioning](https://semver.org/):
- **MAJOR** version when making incompatible API changes
- **MINOR** version when adding functionality in a backwards compatible manner  
- **PATCH** version when making backwards compatible bug fixes

## Release Process

1. Update this CHANGELOG.md with new version section
2. Update version numbers in:
   - `src/main.cpp` (VERSION constant)
   - `CMakeLists.txt` (project version)
3. Create git tag: `git tag -a v0.1.0 -m "Release version 0.1.0"`
4. Push tags: `git push origin --tags`

## Categories

- **Added** for new features
- **Changed** for changes in existing functionality
- **Deprecated** for soon-to-be removed features
- **Removed** for now removed features
- **Fixed** for any bug fixes
- **Security** in case of vulnerabilities