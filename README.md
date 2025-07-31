# GGML Visualizer

[ggmlviz](docs/ggmlviz.md) / [build](docs/BUILD.md)

**Real-time performance visualization for GGML-based LLM runtimes**

## What is GGML Visualizer?

GGML Visualizer is a performance analysis toolkit designed to make understanding Large Language Model inference as simple as watching your model run. The project emerged from the need to debug and optimize GGML-based applications like llama.cpp and whisper.cpp without the traditional complexity of profiling tools that require recompilation, source modifications, or deep systems knowledge.

The core of the tool is **zero-recompilation tracing** through platform-specific function interposition. By using mechanisms like `LD_PRELOAD` on Linux, `DYLD_INSERT_LIBRARIES` on macOS, and MinHook DLL injection on Windows, ggml-viz can intercept GGML function calls in any existing binary and capture detailed execution traces with nanosecond precision. This means you can take any llama.cpp binary you've already built, set a few environment variables, and immediately get real-time visualization of graph computations, individual operation timing, memory allocations, and backend utilization across CPU, Metal, CUDA, and Vulkan.

The captured events flow through a lock-free SPSC (Single Producer, Single Consumer) ring buffer into a custom binary streaming format called GGMLVIZ, designed for high-frequency event capture with minimal overhead. The ring buffer uses cache-line aligned atomic operations with acquire/release memory ordering for thread-safe operation without mutex overhead. The format uses an 8-byte magic header ("GGMLVIZ1"), version information for backward compatibility, and union-based event structures that can be written incrementally during execution and read efficiently during analysis. This enables both post-mortem analysis of saved traces and live monitoring where the visualization GUI updates in real-time as your model runs.

## Project Status and Goals

The project is production-ready on macOS and Linux with a working ImGui-based desktop interface, real-time live mode, and comprehensive build system. The Windows implementation exists but remains experimental due to the complexity of DLL injection mechanisms. The ring buffer now implements a true lock-free SPSC design with proper memory ordering, cache-line alignment, and backpressure monitoring through dropped event counters - replacing the previous mutex-based implementation as detailed in our [ring buffer analysis](docs/RING_BUFFER_ANALYSIS.md).

The primary goal is to make GGML model performance analysis straightforward and accessible for anyone working with GGML-based applications. It's designed to provide clear, real-time insight into tensor computations, operation timings, memory allocations, and backend utilization—without requiring deep systems knowledge or complex profiling setups. The toolkit enables users to observe exactly how their models execute, identify bottlenecks, and understand resource usage at a granular level.

A secondary goal is to showcase advanced C++ techniques for runtime instrumentation, cross-platform interposition, and high-performance event capture. The project demonstrates practical approaches to tracing and visualization in modern C++, serving as both a useful tool and a reference implementation for developers interested in systems programming and performance analysis within the GGML ecosystem.

## Quick Start

```bash
# Build from source (see docs/BUILD.md for details)
git clone --recursive https://github.com/brodheadw/ggml-viz.git
cd ggml-viz && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

# Option 1: Run real model demos (recommended for first-time users)
./examples/run_demos.sh                # Interactive demo menu
./examples/llama_demo/run_llama_demo.sh     # Real LLaMA inference with TinyLlama 1.1B
./examples/whisper_demo/run_whisper_demo.sh # Real Whisper transcription with base.en
./bin/ggml-viz llama_real_trace.ggmlviz     # View real LLaMA trace
./bin/ggml-viz whisper_real_trace.ggmlviz   # View real Whisper trace

# Option 2: Visualize any GGML application (no recompilation needed)
export GGML_VIZ_OUTPUT=trace.ggmlviz
export LD_PRELOAD=build/src/libggml_viz_hook.so  # Linux
# export DYLD_INSERT_LIBRARIES=build/src/libggml_viz_hook.dylib  # macOS
./your_llama_cpp_binary -m model.gguf -p "Hello world" -n 10

# Option 3: Configuration-driven setup
./bin/ggml-viz --config examples/llama_demo/llama_demo_config.json --live trace.ggmlviz
```

## Configuration System

GGML Visualizer now features a comprehensive configuration management system that supports JSON configuration files, environment variables, and CLI flags with proper precedence handling.

### Configuration Methods

**JSON Configuration Files (Recommended)**: Create `.json` files with structured settings for all components:
```json
{
  "cli": {
    "verbose": true,
    "live_mode": true,
    "port": 8080
  },
  "instrumentation": {
    "max_events": 50000,
    "enable_op_timing": true,
    "enable_memory_tracking": true
  },
  "logging": {
    "level": "INFO",
    "enable_timestamps": true
  }
}
```

**Configuration Precedence** (highest to lowest priority):
1. Command-line flags (`--verbose`, `--config`, etc.)
2. JSON configuration files (`--config config.json`)
3. Environment variables (`GGML_VIZ_VERBOSE=1`)
4. Built-in defaults

### Demo Applications

The project includes real model demonstrations that download and run actual GGML applications:

**Real LLaMA Demo** - Downloads and runs TinyLlama 1.1B for actual transformer inference:
```bash
./examples/llama_demo/run_llama_demo.sh  # Downloads model, runs real inference
./bin/ggml-viz llama_real_trace.ggmlviz  # View real computation trace
```

**Real Whisper Demo** - Downloads and runs Whisper base.en for actual audio transcription:
```bash  
./examples/whisper_demo/run_whisper_demo.sh  # Downloads model, transcribes audio
./bin/ggml-viz whisper_real_trace.ggmlviz    # View real audio processing trace
```

**Interactive Demo Runner**:
```bash
./examples/run_demos.sh  # Menu-driven demo selection with requirements
```

These demos show **real GGML operations** including actual attention mechanisms, matrix multiplications, and audio processing - not simulations.

## Integration Approaches

**Built-in Demos (Recommended for Learning)**: Use the included LLaMA and Whisper demonstrations to understand the visualization capabilities and configuration system before integrating with your own applications.

**Function Interposition**: Works with any existing GGML binary by intercepting function calls at runtime. Set environment variables to specify the interposition library path (`LD_PRELOAD` or `DYLD_INSERT_LIBRARIES`) and output trace file (`GGML_VIZ_OUTPUT`), then run your application normally. The interposition library captures events transparently and writes them to the GGMLVIZ format.

**Configuration-Driven Integration**: Use JSON configuration files to control all aspects of tracing and visualization, providing better organization and repeatability than environment variables.

**Live Monitoring**: Run the GUI in live mode (`--live trace.ggmlviz`) to watch events appear in real-time as your application executes. The GUI polls the trace file and updates the visualization automatically, making it possible to see exactly what your model is doing as it processes each token.

## Architecture and Technical Details

The system consists of four main layers: 
- instrumentation (event capture via platform-specific interposition)
- IPC (shared memory communication for cross-process operation)
- Data collection (ring buffer storage and GGMLVIZ format serialization)
- Frontend (ImGui-based real-time visualization) 

Events are captured with nanosecond timestamps, stored in a lock-free SPSC ring buffer with proper acquire/release memory ordering, and periodically flushed to disk in a binary format optimized for both write performance during capture and read performance during analysis. The ring buffer uses the "one empty slot" rule to distinguish full from empty states and implements backpressure through dropped event counters when the buffer is full.

The interposition mechanisms differ by platform but achieve the same goal: intercepting calls to key GGML functions like `ggml_backend_graph_compute()` without modifying the original application. On macOS, this uses Apple's `DYLD_INTERPOSE` mechanism with symbol replacement in the `__DATA,__interpose` section. On Linux, it uses `LD_PRELOAD` for shared library symbol precedence. On Windows, it uses the MinHook library for runtime API patching, though this implementation remains experimental.

Performance impact is designed to be minimal through careful optimization of the event capture path. The lock-free SPSC ring buffer uses monotonic uint64_t counters with masking for indexing, cache-line aligned atomic operations to prevent false sharing, and leaner memory ordering (producer uses relaxed/acquire, consumer uses acquire/relaxed) for optimal performance. Event filtering allows you to capture only the operations you care about, and the binary format avoids expensive serialization during the critical path.

## Current Limitations and Development

The project successfully demonstrates the core concept and provides working visualization for GGML applications, but several areas need continued development. The lock-free SPSC ring buffer is now implemented and production-ready. The Windows implementation requires more robust testing and potentially alternative approaches to DLL injection. The web dashboard server exists but needs frontend development for browser-based visualization.

Advanced features like memory usage tracking, thread synchronization analysis, and plugin-based visualizations are partially implemented but not yet production-ready. Export capabilities to formats like Chrome Trace and Tracy profiler are planned but not yet available. Integration with real-world llama.cpp workflows has been tested manually but needs more comprehensive examples and documentation.

## Contributing and Documentation

The project welcomes contributions across all skill levels, from documentation improvements to core systems programming. The build system is based on CMake with comprehensive CI/CD testing on all supported platforms. The codebase uses modern C++17 with careful attention to cross-platform compatibility and performance optimization.

Key documentation includes the [GGMLVIZ format specification](docs/ggmlviz.md) with complete technical details, the [build guide](docs/BUILD.md) with platform-specific instructions, and the [ring buffer analysis](docs/RING_BUFFER_ANALYSIS.md) documenting current performance issues and solutions. The project follows standard GitHub workflows with issues, pull requests, and releases managed through the repository.

| Platform | Status | Interposition Method | Notes |
|----------|--------|-------------|-------|
| **macOS** (arm64/x64) | ✅ Production | DYLD_INTERPOSE | Full functionality, CI tested |
| **Linux** (x64) | ✅ Production | LD_PRELOAD | Full functionality, CI tested |
| **Windows** 10+ | ⚠️ Experimental | MinHook DLL | Basic functionality, needs testing |
| **Raspberry Pi** | ❓ Untested | LD_PRELOAD | Should work but unverified |