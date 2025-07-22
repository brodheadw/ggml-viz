# GGML Visualizer

> **A cross-platform, real-time dashboard for visualizing GGML-based LLM runtimes like llama.cpp and whisper.cpp with Metal, CUDA, and CPU backend support.**

---

## 🚀 Quick Start (2 Minutes)

### Prerequisites

```bash
# macOS
brew install cmake glfw

# Ubuntu/Debian
sudo apt update && sudo apt install -y git cmake build-essential libgl1-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev
```

### Build

```bash
git clone --recursive https://github.com/your-org/ggml-visualizer
cd ggml-visualizer
mkdir build && cd build

# macOS (Metal disabled due to shader issues)
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF
make -j4

# Linux
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Windows (PowerShell/CMD)
cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel
```

### Test the Installation

```bash
# Unix/Linux/macOS
./bin/ggml-viz --version
./bin/ggml-viz --help
./tests/manual/test_ggml_hook
./bin/ggml-viz tests/assets/test_trace.ggmlviz
```

```powershell
# Windows (PowerShell)
.\bin\Release\ggml-viz.exe --version
.\bin\Release\ggml-viz.exe --help
.\bin\Release\test_ggml_hook.exe
.\bin\Release\ggml-viz.exe tests\assets\test_trace.ggmlviz
```

---

## 📖 Complete Tutorial

### Method 1: External Hook Injection (Recommended)

This method works with **any existing llama.cpp installation** without recompiling.

#### Step 1: Download a Test Model
```bash
# Download a small model for testing
mkdir -p models && cd models
wget https://huggingface.co/microsoft/DialoGPT-medium/resolve/main/pytorch_model.bin
# Or use any .gguf model you have
```

#### Step 2: Start the GUI (Terminal 1)
```bash
# Start GUI in live mode, monitoring trace.ggmlviz
./bin/ggml-viz --live tests/traces/trace.ggmlviz --no-hook --verbose
```

You should see:
```
[ImGuiApp] Live mode enabled with built-in hook disabled (--no-hook)
[ImGuiApp] Monitoring specified trace file: tests/traces/trace.ggmlviz
```

#### Step 3: Run llama.cpp with Hooks (Terminal 2)

**Unix/Linux/macOS:**
```bash
# Set environment variables
export GGML_VIZ_OUTPUT=tests/traces/trace.ggmlviz
export DYLD_INSERT_LIBRARIES=build/src/libggml_viz_hook.dylib  # macOS
export LD_PRELOAD=build/src/libggml_viz_hook.so               # Linux

# Run any llama.cpp binary with hooks
./third_party/llama.cpp/build/bin/llama-cli \
  -m models/your-model.gguf \
  -p "Hello, world!" \
  -n 10 \
  --no-display-prompt
```

**Windows (PowerShell):**
```powershell
# Set environment variables
$env:GGML_VIZ_OUTPUT = "tests\traces\trace.ggmlviz"
$env:GGML_VIZ_VERBOSE = "1"

# Windows uses DLL injection automatically via DllMain
.\third_party\llama.cpp\build\bin\llama-cli.exe `
  -m models\your-model.gguf `
  -p "Hello, world!" `
  -n 10 `
  --no-display-prompt
```

#### Step 4: Watch Real-Time Visualization
The GUI automatically updates as events are captured. You should see:
- **Hook Output**: `[GGML_VIZ] *** SCHEDULER INTERPOSED ***` messages
- **GUI Updates**: `[ImGuiApp] Loaded X new events from external file`
- **Live Dashboard**: Real-time graph and timeline visualization

### Method 2: Direct Integration (Advanced)

For custom GGML applications or development purposes.

#### Step 1: Link Against ggml-viz
```cmake
# In your CMakeLists.txt
find_package(ggml_viz REQUIRED)
target_link_libraries(your_app ggml_viz_hook)
```

#### Step 2: Initialize in Code
```cpp
#include "ggml_hook.hpp"

int main() {
    // Start instrumentation
    auto& hook = ggml_viz::GGMLHook::instance();
    hook.start();
    
    // Your GGML code here
    ggml_graph_compute(ctx, graph);
    
    // Stop instrumentation
    hook.stop();
    return 0;
}
```

#### Step 3: Set Output and Run
```bash
export GGML_VIZ_OUTPUT=my_trace.ggmlviz
./your_app

# Visualize results
./bin/ggml-viz my_trace.ggmlviz
```

---

## 🛠 Advanced Usage

### Live Mode with Custom File Paths
```bash
# Terminal 1: GUI monitoring specific file
./bin/ggml-viz --live /path/to/my_trace.ggmlviz --no-hook

# Terminal 2: Application writing to same file
export GGML_VIZ_OUTPUT=/path/to/my_trace.ggmlviz
export DYLD_INSERT_LIBRARIES=build/src/libggml_viz_hook.dylib
./your_ggml_app
```

### Web Server Mode (Experimental)
```bash
# Start web server
./bin/ggml-viz --web --port 8080

# Access dashboard at http://localhost:8080
```

### Batch Processing Multiple Traces
```bash
# Process all traces in directory
for trace in tests/traces/*.ggmlviz; do
    echo "Processing $trace..."
    ./bin/ggml-viz "$trace" --export-stats
done
```

### Performance Benchmarking
```bash
# Quick performance check
./scripts/simple_benchmark.sh

# Detailed benchmarking with overhead measurement
./scripts/benchmark.sh
```

**Performance Impact**: Overhead measurement in progress.

---

## 🎯 Environment Variables

### Essential Variables
- **`GGML_VIZ_OUTPUT`**: Output trace file path (required for capture)
- **`DYLD_INSERT_LIBRARIES`**: macOS library injection path
- **`LD_PRELOAD`**: Linux library injection path  
- **Windows**: Uses automatic DLL injection via DllMain (no manual injection needed)

### Configuration Variables
- **`GGML_VIZ_VERBOSE`**: Enable verbose logging output
- **`GGML_VIZ_DISABLE`**: Disable instrumentation entirely  
- **`GGML_VIZ_MAX_EVENTS`**: Maximum events to capture (default: 10,000,000)

### Advanced Debugging
- **`GGML_VIZ_OP_TIMING`**: Enable operation timing (default: true)
- **`GGML_VIZ_MEMORY_TRACKING`**: Enable memory tracking (default: false)
- **`GGML_VIZ_THREAD_TRACKING`**: Enable thread tracking (default: false)
- **`GGML_VIZ_TENSOR_NAMES`**: Capture tensor names (default: true)

---

## 📊 Understanding the Output

### Trace File Format
Generated `.ggmlviz` files contain:
- **Header**: Magic bytes "GGMLVIZ1" + metadata
- **Events**: Binary event stream with timestamps
- **Statistics**: Performance metrics and summaries

### Event Types Captured
- **Graph Compute**: Begin/end of graph execution
- **Operation Timing**: Individual operation performance
- **Memory Events**: Allocation/deallocation tracking
- **Backend Events**: GPU kernel launches and synchronization

### GUI Components
- **Timeline View**: Flame chart showing operation execution
- **Graph View**: Static computation graph visualization  
- **Memory View**: Memory usage and allocation patterns
- **Statistics Panel**: Performance metrics and bottlenecks

---

## 🔧 Troubleshooting

### Common Issues

#### "No events captured"
```bash
# Check if hooks are loaded
export GGML_VIZ_VERBOSE=1
# Look for "[GGML_VIZ] Hook started" messages
```

#### "Failed to read trace header"
```bash
# Verify file exists and has correct format
hexdump -C your_trace.ggmlviz | head -1
# Should start with: 47 47 4d 4c 56 49 5a 31 (GGMLVIZ1)
```

#### macOS "Library not loaded"
```bash
# Fix library path
export DYLD_INSERT_LIBRARIES=$(pwd)/build/src/libggml_viz_hook.dylib
# Use absolute path instead of relative
```

#### GUI not updating in live mode
```bash
# Ensure both processes use the same file path
./bin/ggml-viz --live trace.ggmlviz --no-hook  # Terminal 1
export GGML_VIZ_OUTPUT=trace.ggmlviz           # Terminal 2
```

### Debug Commands
```bash
# Test hook system
./tests/manual/test_ggml_hook

# Verify library symbols
nm build/src/libggml_viz_hook.dylib | grep viz_sched

# Check file permissions
ls -la tests/traces/trace.ggmlviz

# Monitor file changes
tail -f tests/traces/trace.ggmlviz
```

---

## 🏗 Architecture

### Core Components

```mermaid
graph TD
    A[GGML Application] -->|DYLD_INTERPOSE| B[Scheduler Hooks]
    B --> C[Event Capture]
    C --> D[Ring Buffer]
    D --> E[Binary Trace File]
    E --> F[GUI Monitoring]
    F --> G[Real-time Visualization]
```

- **Scheduler Interposition**: DYLD_INTERPOSE for guaranteed symbol replacement
- **Event Capture**: Lock-free ring buffer for high-performance recording
- **Binary Format**: Efficient `.ggmlviz` format with version headers
- **Live Monitoring**: File-based communication between processes
- **Cross-Platform**: Works with Metal, CUDA, CPU, and Vulkan backends

### Supported Platforms

| Platform | CPU | GPU | Hook Method | Build Status | Functionality |
|----------|-----|-----|-------------|--------------|---------------|
| macOS (arm64/x64) | ✅ AVX2/NEON | ✅ Metal* | DYLD_INTERPOSE | ✅ Complete | ✅ Core features working |
| Linux (x64) | ✅ AVX2/AVX-512 | ✅ CUDA/Vulkan | LD_PRELOAD | ✅ Complete | ✅ Build system working |
| Windows 10+ | ✅ AVX2 | ✅ CUDA/DirectML | MinHook DLL Injection | ✅ Complete | ✅ Build system working |
| Raspberry Pi | ✅ NEON | ❌ | LD_PRELOAD | ❓ Untested | ❓ Unknown |

*Metal backend requires `-DGGML_METAL=OFF` due to shader compilation issues

---

## 🧪 Development

### Running Tests
```bash
# Unit tests
cd build && ctest

# Manual tests
./tests/manual/test_ggml_hook
./tests/manual/test_interpose

# Integration tests
./tests/integration/demo_live_mode_with_llama.sh
```

### Contributing

We welcome contributions! Please see [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) for:

- Development setup instructions
- Code style guidelines and automated formatting tools
- Testing procedures
- Current development priorities (see [TODO.md](TODO.md))
- Pull request process

### Development Commands
```bash
# Code quality
./scripts/format.sh     # Auto-format C++ and CMake files
./scripts/lint.sh       # Static analysis and code linting
./scripts/run_tests.sh  # Run test suite

# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug -DGGML_METAL=OFF
make -j4

# Test with verbose output
./bin/ggml-viz --verbose tests/traces/trace.ggmlviz
```

---

## 📝 Status & Roadmap

### ✅ **Core Systems Working**
- ✅ Build system and CI for Windows, macOS, Linux
- ✅ Basic instrumentation and event capture system
- ✅ ImGui desktop visualization interface (basic functionality)
- ✅ Binary trace format with version headers
- ✅ CLI interface with --help, --version, basic options
- ✅ External hook injection mechanisms implemented
- ✅ Cross-platform compilation and testing infrastructure

### 🚧 **Partial/Needs Work**
- 🚧 Live mode functionality (CLI exists, backend incomplete)
- 🚧 Real-time visualization updates (basic implementation)
- 🚧 Cross-platform hook testing (Windows/Linux need validation)
- 🚧 Integration with real llama.cpp workflows

### ❌ **Major Features Missing**
- ❌ Complete live mode backend implementation
- ❌ Advanced timeline visualization
- ❌ Tensor inspection and statistics
- ❌ Memory usage tracking
- ❌ Web dashboard functionality
- ❌ Plugin system
- ❌ Real-world integration examples (llama.cpp, whisper.cpp)

### 📋 **Planned**
- 📋 Web dashboard (browser-based interface)
- 📋 Plugin system for custom visualizations
- 📋 Export capabilities (SVG, JSON, CSV)
- 📋 Integration with profiling tools (Tracy, perf)
- 📋 Advanced Windows debugging tools integration

---

## 📄 License

Licensed under **Apache 2.0**. See [LICENSE](LICENSE) for details.

Third-party dependencies are listed in [docs/THIRD_PARTY.md](docs/THIRD_PARTY.md).

---

## 🙏 Credits

- **Georgi Gerganov** and the GGML community for the foundation
- **llama.cpp contributors** for the excellent inference engine
- **ImGui team** for the immediate mode GUI framework
- **Tracy profiler** for real-time profiling inspiration

---

*"The best way to understand your model is to watch it run."*