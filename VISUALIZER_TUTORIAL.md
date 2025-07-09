# GGML Visualizer Tutorial

A comprehensive guide to building, configuring, and running the GGML Visualizer for real-time monitoring of GGML-based LLM inference.

## Prerequisites

### System Requirements
- **macOS**: Apple Silicon (M1/M2) or Intel
- **Linux**: Ubuntu 20.04+ or equivalent
- **Windows**: Windows 10+ (experimental support)

### Dependencies
```bash
# macOS
brew install cmake glfw

# Ubuntu/Debian
sudo apt update && sudo apt install -y git cmake build-essential libgl1-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev

# Windows (using vcpkg)
vcpkg install glfw3 opengl
```

## Building the Visualizer

### 1. Clone and Setup
```bash
git clone <repository-url>
cd ggml-viz
git submodule update --init --recursive
```

### 2. Build Configuration
```bash
mkdir build && cd build

# Standard Release build (recommended for macOS)
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF

# Linux build with full backends
cmake .. -DCMAKE_BUILD_TYPE=Release

# Debug build with symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug -DGGML_METAL=OFF
```

### 3. Compile
```bash
make -j$(nproc)  # Linux
make -j4         # macOS
```

### 4. Verify Build
```bash
# Test the visualizer binary
./bin/ggml-viz --help

# Test the hook system
./bin/test_ggml_hook

# Verify hook library exists
ls -la src/libggml_viz_hook.dylib  # macOS
ls -la src/libggml_viz_hook.so     # Linux
```

If rebuilding, unset the environmental variables first:
```bash
unset GGML_VIZ_OUTPUT && unset DYLD_INSERT_LIBRARIES && unset DYLD_FORCE_FLAT_NAMESPACE
```

## Usage Methods

### Method 1: External Process Monitoring (Recommended)

This method captures trace data from any GGML-based application without modifying its source code.

#### Step 1: Set Environment Variables
```bash
# Set output file for trace data
export GGML_VIZ_OUTPUT=my_trace.ggmlviz

# macOS: Enable dynamic library injection
export DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib
export DYLD_FORCE_FLAT_NAMESPACE=1

# Linux: Enable shared library injection
export LD_PRELOAD=./build/src/libggml_viz_hook.so
```

#### Step 2: Run Your GGML Application
```bash
# Example with llama.cpp
./third_party/llama.cpp/build/bin/llama-cli -m model.gguf -p "Hello, world!" -n 50

# Example with whisper.cpp
/third_party/whisper.cpp/main -m model.bin -f audio.wav

# Example with any other GGML application
./your_ggml_app --your-args
```

#### Step 3: Launch Live Visualizer
```bash
# Start live monitoring (monitors the trace file for updates)
./build/bin/ggml-viz --live my_trace.ggmlviz --no-hook

# Or load completed trace file
./build/bin/ggml-viz my_trace.ggmlviz
```

### Method 2: Built-in Hook Mode

This method uses the visualizer's built-in hooks for direct monitoring.

```bash
# Launch with built-in hooks enabled
./build/bin/ggml-viz --live trace.ggmlviz

# In another terminal, run your application
export GGML_VIZ_OUTPUT=trace.ggmlviz
./your_ggml_app --your-args
```

### Method 3: Offline Analysis

Analyze pre-recorded trace files.

```bash
# Load existing trace file
./build/bin/ggml-viz trace.ggmlviz

# Load with verbose output
./build/bin/ggml-viz --verbose trace.ggmlviz
```

## Complete llama.cpp Integration Example

### Step 1: Build llama.cpp
```bash
# Clone and build llama.cpp
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
make -j4
```

### Step 2: Download a Model
```bash
# Download a small model for testing
wget https://huggingface.co/microsoft/DialoGPT-medium/resolve/main/pytorch_model.bin
# Or use any GGUF model you have
```

### Step 3: Start the Visualizer
```bash
cd /path/to/ggml-viz

# Set environment variables
export GGML_VIZ_OUTPUT=llama_trace.ggmlviz
export DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib  # macOS
export DYLD_FORCE_FLAT_NAMESPACE=1                              # macOS

# Start live monitoring
./build/bin/ggml-viz --live llama_trace.ggmlviz --no-hook
```

### Step 4: Run llama.cpp Inference
```bash
# In a separate terminal
cd /path/to/llama.cpp
export GGML_VIZ_OUTPUT=llama_trace.ggmlviz
export DYLD_INSERT_LIBRARIES=/path/to/ggml-viz/build/src/libggml_viz_hook.dylib
export DYLD_FORCE_FLAT_NAMESPACE=1

./main -m your_model.gguf -p "Explain quantum computing" -n 100
```

## GUI Features

### Timeline View
- **Visual Timeline**: Interactive timeline showing operation execution
- **Events Tab**: Detailed list of all recorded events
- **Op Timings**: Performance breakdown by operation type

### Graph View
- **Compute Graph**: Visual representation of the computation graph
- **Node Selection**: Click nodes to inspect details
- **Live Updates**: Real-time graph updates during inference

### Tensor Inspector
- **Event Details**: Inspect individual events and their metadata
- **Tensor Information**: View tensor shapes, types, and properties
- **Performance Metrics**: Per-tensor timing information

### Memory View
- **Memory Usage**: Track memory allocation and deallocation
- **Memory Timeline**: Visual memory usage over time
- **Memory Leaks**: Identify potential memory issues

## Environment Variables

### Essential Variables
- `GGML_VIZ_OUTPUT`: Output trace file path (required)
- `DYLD_INSERT_LIBRARIES`: Path to hook library (macOS)
- `LD_PRELOAD`: Path to hook library (Linux)
- `DYLD_FORCE_FLAT_NAMESPACE`: Enable symbol interposition (macOS)

### Optional Configuration
- `GGML_VIZ_VERBOSE`: Enable verbose logging
- `GGML_VIZ_DISABLE`: Disable instrumentation entirely
- `GGML_VIZ_MAX_EVENTS`: Maximum events to capture (default: 10,000,000)
- `GGML_VIZ_OP_TIMING`: Enable operation timing (default: true)
- `GGML_VIZ_MEMORY_TRACKING`: Enable memory tracking (default: false)
- `GGML_VIZ_THREAD_TRACKING`: Enable thread tracking (default: false)
- `GGML_VIZ_TENSOR_NAMES`: Capture tensor names (default: true)

## Command Line Options

### Basic Usage
```bash
./build/bin/ggml-viz [OPTIONS] [TRACE_FILE]
```

### Options
- `--live`: Enable live monitoring mode
- `--no-hook`: Disable built-in hooks (for external monitoring)
- `--verbose`: Enable verbose output
- `--help`: Show help message
- `--version`: Show version information
- `--port PORT`: Set port for live mode (default: 8080)

## Troubleshooting

### Build Issues

**macOS Metal Shader Issues**
```bash
# Use -DGGML_METAL=OFF to disable Metal backend
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF
```

**OpenMP Warning**
```bash
# This warning is normal and doesn't affect functionality
# OpenMP not found warning can be ignored
```

### Runtime Issues

**No Events Captured**
```bash
# Check environment variables
echo $GGML_VIZ_OUTPUT
echo $DYLD_INSERT_LIBRARIES

# Verify hook library exists
ls -la ./build/src/libggml_viz_hook.dylib

# Test with built-in test
./build/bin/test_ggml_hook
```

**Empty Trace File**
```bash
# Check file permissions
ls -la trace.ggmlviz

# Verify application is using GGML scheduler functions
# The hook intercepts ggml_backend_sched_graph_compute functions
```

**GUI Not Starting**
```bash
# Check OpenGL support
# Ensure GLFW is properly installed
# Try running in compatibility mode
```

### Permission Issues (macOS)

```bash
# If you get security warnings about the dylib
sudo spctl --add ./build/src/libggml_viz_hook.dylib
sudo spctl --enable
```

## Performance Considerations

### Memory Usage
- Default buffer size: 65,536 events
- Each event: ~64 bytes
- Total memory: ~4MB for event buffer
- Trace files: ~1KB per 1000 events

### Performance Impact
- Overhead: <5% in most cases
- File I/O: Batched every 4096 events
- Lock-free ring buffer for minimal contention

### Optimization Tips
```bash
# Reduce event capture for better performance
export GGML_VIZ_MAX_EVENTS=100000

# Disable memory tracking if not needed
export GGML_VIZ_MEMORY_TRACKING=false

# Disable tensor name capture for speed
export GGML_VIZ_TENSOR_NAMES=false
```

## Advanced Usage

### Custom Integration
```cpp
#include "instrumentation/ggml_hook.hpp"

// In your application
auto& hook = ggml_viz::GGMLHook::instance();
hook.start();
// ... your GGML code ...
hook.stop();
```

### Batch Processing
```bash
# Process multiple trace files
for trace in traces/*.ggmlviz; do
    ./build/bin/ggml-viz "$trace" --export-csv "${trace%.ggmlviz}.csv"
done
```

### Network Monitoring
```bash
# Start visualizer with network interface
./build/bin/ggml-viz --live --port 8080
# Access via http://localhost:8080 (when implemented)
```

## File Formats

### Trace File Format (.ggmlviz)
- **Magic Header**: "GGMLVIZ1" (8 bytes)
- **Version**: Binary version number
- **Events**: Sequential binary event records
- **Endianness**: Little-endian

### Event Structure
```cpp
struct Event {
    EventType type;           // 4 bytes
    uint64_t timestamp_ns;    // 8 bytes
    uint32_t thread_id;       // 4 bytes
    char label[48];           // 48 bytes (null-terminated)
    // Total: 64 bytes
};
```

## Getting Help

### Debug Information
```bash
# Enable verbose logging
export GGML_VIZ_VERBOSE=1

# Check system information
./build/bin/ggml-viz --version

# Validate trace file
./build/bin/test_trace_reader your_trace.ggmlviz
```

### Common Issues
1. **No events captured**: Check that your application uses GGML scheduler functions
2. **GUI crashes**: Verify OpenGL/GLFW installation
3. **Performance issues**: Reduce event capture or disable unnecessary features
4. **File permissions**: Ensure write access to trace file location

### Support
- Check the project README for latest updates
- Review the troubleshooting section in CLAUDE.md
- Run the test suite to verify installation
- Use verbose mode for detailed diagnostic information

This tutorial covers the complete workflow from building to running the GGML visualizer. For specific use cases or advanced configurations, refer to the individual documentation files in the docs/ directory.