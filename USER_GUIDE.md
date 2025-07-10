# GGML Visualizer User Guide

Real-time visualization and analysis of GGML-based language model inference. See your models compute in action with detailed operation timelines and computation graphs.

## Table of Contents

1. [macOS Quick Start](#macos-quick-start)
2. [Linux Setup](#linux-setup)
3. [Windows Setup](#windows-setup)
4. [Understanding the Interface](#understanding-the-interface)
5. [Advanced Usage](#advanced-usage)
6. [Troubleshooting](#troubleshooting)

---

## macOS Quick Start

### Prerequisites

```bash
# Install dependencies
brew install cmake glfw
```

### Build GGML Visualizer

```bash
# Clone and build
git clone --recursive https://github.com/your-org/ggml-visualizer.git
cd ggml-visualizer
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF
make -j4

# Verify build
./bin/ggml-viz --help
```

### Method 1: Live Mode (Recommended)

Watch your model compute in real-time as it generates tokens:

**Terminal 1 (Start the GUI):**
```bash
./build/bin/ggml-viz --live test_live_trace.ggmlviz --no-hook
```

**Terminal 2 (Run your model with hooks):**
```bash
env GGML_VIZ_OUTPUT=test_live_trace.ggmlviz \
    DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    ./third_party/llama.cpp/build/bin/llama-cli \
    -m ./models/your-model.gguf \
    -p "Hello world" \
    -n 10 \
    --verbose-prompt
```

**What you'll see:**
- **Real-time updates**: Events appear as they happen (~1,500 operations per token)
- **Timeline View**: Detailed operation timeline showing matrix multiplies, additions, etc.
- **Graph View**: Visual representation of the neural network computation graph
- **Live statistics**: Token generation speed, operation counts, timing data

### Method 2: Offline Analysis

Capture trace data first, then analyze:

```bash
# 1. Capture inference data
export GGML_VIZ_OUTPUT=my_trace.ggmlviz
env DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    ./third_party/llama.cpp/build/bin/llama-cli \
    -m ./models/your-model.gguf \
    -p "Explain quantum computing" \
    -n 50

# 2. Analyze the captured data
./build/bin/ggml-viz my_trace.ggmlviz
```

### Setting Up llama.cpp with Hooks

If you don't have llama.cpp built with shared libraries:

```bash
# Build llama.cpp with shared GGML libraries
cd third_party/llama.cpp
cmake -B build -DLLAMA_STATIC=OFF -DGGML_BUILD_CPP_SHARED=ON
cmake --build build --config Release
```

### Example Session

```bash
# Start live visualization
./build/bin/ggml-viz --live chat_session.ggmlviz --no-hook &

# Run an interactive chat session with visualization
env GGML_VIZ_OUTPUT=chat_session.ggmlviz \
    DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    ./third_party/llama.cpp/build/bin/llama-cli \
    -m ./models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
    -p "You are a helpful assistant. User: " \
    -i
```

### Environment Variables

**Required:**
- `GGML_VIZ_OUTPUT`: Path to output trace file

**Optional:**
- `GGML_VIZ_VERBOSE`: Enable debug output
- `GGML_VIZ_MAX_EVENTS`: Limit captured events (default: 10M)
- `GGML_VIZ_DISABLE`: Disable instrumentation entirely

---

## Linux Setup

> **TODO**: Linux instructions coming soon
> 
> The basic process will be similar to macOS but using:
> - `apt install` or `yum install` for dependencies
> - `LD_PRELOAD` instead of `DYLD_INSERT_LIBRARIES`
> - Different library paths and build configurations

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update && sudo apt install -y git cmake build-essential \
    libgl1-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev

# CentOS/RHEL
# TODO: Add package manager commands
```

### Build Instructions
```bash
# TODO: Add Linux-specific build steps
```

### Usage
```bash
# TODO: Add LD_PRELOAD commands for Linux
```

---

## Windows Setup

> **TODO**: Windows instructions coming soon
>
> Windows support will include:
> - Visual Studio build instructions
> - DLL injection methods
> - Windows-specific GUI considerations

### Prerequisites
```cmd
REM TODO: Add Windows dependency installation
```

### Build Instructions
```cmd
REM TODO: Add Windows build steps
```

### Usage
```cmd
REM TODO: Add Windows hook injection commands
```

---

## Understanding the Interface

### Timeline View

The Timeline View shows operations as they execute over time:

- **Green bars**: Matrix multiplication operations (usually the slowest)
- **Blue bars**: Addition and normalization operations  
- **Purple bars**: Attention computations
- **Yellow bars**: Activation functions (ReLU, GELU, etc.)
- **Red bars**: Memory operations

**What to look for:**
- Long green bars indicate matrix multiply bottlenecks
- Gaps between operations suggest synchronization issues
- Repeated patterns show the layer structure of your model

### Graph View

The Graph View shows the computation graph structure:

- **Nodes**: Individual operations (matmul, add, softmax, etc.)
- **Edges**: Data flow between operations
- **Colors**: Operation types
- **Size**: Relative computational cost

**What to look for:**
- **Transformer blocks**: Repeated attention + feedforward patterns
- **Bottleneck operations**: Large nodes that take significant time
- **Data dependencies**: How operations must wait for each other

### Statistics Panel

Key metrics displayed:
- **Total Events**: Number of operations captured (~1,500 per token for Llama)
- **Duration**: Total time for inference
- **Tokens/Second**: Generation speed
- **Graph Events**: High-level forward passes
- **Operation Events**: Individual tensor computations

### Understanding the Numbers

**For a typical Llama-7B model:**
- ~1,500 operations per token (matrix mults, adds, etc.)
- ~50-100ms per token on M1 Max
- ~70% time in matrix multiplications
- ~20 graph events (forward passes during initialization + generation)

**For smaller models like TinyLlama-1.1B:**
- ~800-1,200 operations per token  
- ~10-20ms per token on M1 Max
- Similar operation distribution but faster execution

---

## Advanced Usage

### Performance Analysis

**Identify bottlenecks:**
```bash
# Capture a performance trace
env GGML_VIZ_OUTPUT=perf_analysis.ggmlviz \
    DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    ./third_party/llama.cpp/build/bin/llama-cli \
    -m your_model.gguf \
    -p "Generate some text to analyze" \
    -n 100

# Analyze in GUI - look for:
# 1. Longest operations in Timeline View
# 2. Most expensive nodes in Graph View  
# 3. Patterns in operation timing
```

**Compare models:**
```bash
# Test different model sizes
for model in tinyllama-1b llama-7b llama-13b; do
    env GGML_VIZ_OUTPUT=${model}_trace.ggmlviz \
        DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
        ./third_party/llama.cpp/build/bin/llama-cli \
        -m models/${model}.gguf \
        -p "Standard test prompt" \
        -n 20
done
```

**Backend comparison:**
```bash
# Compare CPU vs Metal performance
export GGML_VIZ_OUTPUT=cpu_trace.ggmlviz
env GGML_METAL=0 DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    ./third_party/llama.cpp/build/bin/llama-cli -m model.gguf -p "test" -n 10

export GGML_VIZ_OUTPUT=metal_trace.ggmlviz  
env GGML_METAL=1 DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    ./third_party/llama.cpp/build/bin/llama-cli -m model.gguf -p "test" -n 10
```

### Debugging Model Issues

**Analyze poor generation quality:**
```bash
# Capture trace with verbose output for debugging
env GGML_VIZ_OUTPUT=debug_trace.ggmlviz \
    GGML_VIZ_VERBOSE=1 \
    DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    ./third_party/llama.cpp/build/bin/llama-cli \
    -m problematic_model.gguf \
    -p "Test prompt that gives bad output" \
    -n 20 \
    --verbose-prompt

# Look for:
# - Unusual operation patterns
# - Unexpected tensor shapes
# - Timing anomalies
```

### Production Monitoring

**Minimal overhead capture:**
```bash
# Lightweight monitoring for production
env GGML_VIZ_OUTPUT=prod_monitor.ggmlviz \
    GGML_VIZ_MAX_EVENTS=50000 \
    DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    your_production_command
```

---

## Troubleshooting

### Common Issues

**1. "No events captured" / Empty timeline**

Check your environment variables:
```bash
echo $GGML_VIZ_OUTPUT  # Should show your trace file path
echo $DYLD_INSERT_LIBRARIES  # Should show path to libggml_viz_hook.dylib
```

Verify the hook library exists:
```bash
ls -la ./build/src/libggml_viz_hook.dylib
```

**2. "Hook library not loading"**

Make sure you're using the correct llama.cpp build:
```bash
# Check if llama.cpp uses shared GGML libraries
otool -L third_party/llama.cpp/build/bin/llama-cli | grep ggml
# Should show libggml.dylib, not static linking
```

**3. "GUI shows no data in live mode"**

Verify the trace file is growing:
```bash
# In another terminal while model is running
watch -n 1 "ls -la test_live_trace.ggmlviz"
# File size should increase as model generates tokens
```

**4. "Too many events / GUI is slow"**

Reduce captured events:
```bash
env GGML_VIZ_OUTPUT=trace.ggmlviz \
    GGML_VIZ_MAX_EVENTS=100000 \
    DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    your_command
```

**5. Build errors on macOS**

Disable Metal if you get shader compilation errors:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF
```

### Debug Commands

```bash
# Test the hook system
./build/bin/test_ggml_hook

# Verify trace file format
file your_trace.ggmlviz  # Should show binary data

# Test trace reading
./build/bin/test_trace_reader your_trace.ggmlviz

# Enable maximum debugging
env GGML_VIZ_VERBOSE=1 \
    GGML_VIZ_OUTPUT=debug.ggmlviz \
    DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \
    your_command 2>&1 | tee debug.log
```

### Getting Help

1. **Check the console output** - Look for `[GGML_VIZ]` messages
2. **Verify your setup** - Use the test commands above
3. **Start simple** - Try with TinyLlama first, then larger models
4. **Check file permissions** - Make sure you can write to the output directory

---

## What You Should See

**Successful live mode session:**
- Console: `[ImGuiApp] Loaded XXX new events from external file`
- GUI Timeline: Dense timeline with hundreds of colored bars per token
- GUI Graph: Network diagram showing model architecture
- Real-time updates as you type prompts to the model

Around ~12,000 lines

The visualization gives you unprecedented insight into how language models actually compute. Use it to optimize performance, debug issues, and understand model behavior at the operation level.

Happy visualizing! 🚀