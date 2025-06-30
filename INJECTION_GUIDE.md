# GGML Visualizer - Library Injection Guide

This guide explains how to use the GGML Visualizer with existing applications like llama.cpp and whisper.cpp through automatic library injection.

## 🚀 Quick Start

### 1. Build the Project

```bash
cd ggml-viz
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)  # or make -j4 on macOS
```

### 2. Run with Injection

**macOS:**
```bash
./scripts/inject_macos.sh llama.cpp/main -m model.gguf -p "Hello, world!"
```

**Linux:**
```bash
./scripts/inject_linux.sh llama.cpp/main -m model.gguf -p "Hello, world!"
```

**Windows:**
```cmd
scripts\inject_windows.bat llama.cpp\main.exe -m model.gguf -p "Hello, world!"
```

### 3. View Results

```bash
./build/bin/ggml-viz ggml_trace_*.ggmlviz
```

## 📋 Environment Variables

Control the instrumentation behavior with these environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `GGML_VIZ_OUTPUT` | auto-generated | Output trace file path |
| `GGML_VIZ_VERBOSE` | `0` | Enable verbose logging (1/0) |
| `GGML_VIZ_OP_TIMING` | `1` | Enable operation timing (1/0) |
| `GGML_VIZ_MEMORY_TRACKING` | `0` | Enable memory tracking (1/0) |
| `GGML_VIZ_TENSOR_NAMES` | `1` | Enable tensor names (1/0) |
| `GGML_VIZ_MAX_EVENTS` | `10000000` | Maximum events to record |
| `GGML_VIZ_DISABLE` | `0` | Disable instrumentation (1/0) |

### Examples

**Custom output file:**
```bash
GGML_VIZ_OUTPUT=my_trace.ggmlviz ./scripts/inject_macos.sh llama.cpp/main -m model.gguf -p "Test"
```

**Verbose logging:**
```bash
GGML_VIZ_VERBOSE=1 ./scripts/inject_macos.sh llama.cpp/main -m model.gguf -p "Test"
```

**Memory tracking enabled:**
```bash
GGML_VIZ_MEMORY_TRACKING=1 ./scripts/inject_macos.sh llama.cpp/main -m model.gguf -p "Test"
```

**Limited event count:**
```bash
GGML_VIZ_MAX_EVENTS=1000 ./scripts/inject_macos.sh llama.cpp/main -m model.gguf -p "Test"
```

## 🔧 How It Works

### Library Injection

The GGML Visualizer uses **function interception** to capture GGML operations without modifying existing applications:

1. **Shared Library**: `libggml_viz_hook.dylib/.so/.dll` contains our instrumentation code
2. **Function Override**: We override key GGML functions like `ggml_backend_graph_compute`
3. **Automatic Init**: The library automatically initializes when loaded
4. **Event Capture**: GGML operations are captured and logged to a trace file

### Platform-Specific Injection

**macOS**: Uses `DYLD_INSERT_LIBRARIES` to inject our shared library
**Linux**: Uses `LD_PRELOAD` to inject our shared library  
**Windows**: Adds library directory to PATH (more complex injection possible)

## 📊 Usage Examples

### llama.cpp Integration

```bash
# Simple text generation
./scripts/inject_macos.sh llama.cpp/main \
  -m models/llama-7b.gguf \
  -p "The capital of France is" \
  -n 32

# Interactive mode with memory tracking
GGML_VIZ_MEMORY_TRACKING=1 \
./scripts/inject_macos.sh llama.cpp/main \
  -m models/llama-7b.gguf \
  -i --interactive-first

# Custom output file
GGML_VIZ_OUTPUT=llama_generation.ggmlviz \
./scripts/inject_macos.sh llama.cpp/main \
  -m models/llama-7b.gguf \
  -p "Write a story about AI" \
  -n 256
```

### whisper.cpp Integration

```bash
# Audio transcription
./scripts/inject_macos.sh whisper.cpp/main \
  -m models/ggml-base.en.bin \
  -f audio.wav

# With verbose logging
GGML_VIZ_VERBOSE=1 \
./scripts/inject_macos.sh whisper.cpp/main \
  -m models/ggml-base.en.bin \
  -f audio.wav \
  --output-txt
```

### Custom Applications

Any application using GGML can be instrumented:

```bash
# Your custom GGML application
./scripts/inject_macos.sh ./my_ggml_app --config config.json

# With minimal event capture
GGML_VIZ_MAX_EVENTS=1000 \
./scripts/inject_macos.sh ./my_ggml_app
```

## 🔍 Analyzing Results

After running with instrumentation, analyze the trace file:

### GUI Viewer

```bash
./build/bin/ggml-viz trace_file.ggmlviz
```

Features:
- **Timeline View**: See operation sequences over time
- **Graph View**: Visualize compute graph structure  
- **Tensor Inspector**: Examine tensor details
- **Performance Analysis**: Identify bottlenecks

### Command Line Analysis

```bash
# Basic statistics
./build/bin/ggml-viz --stats trace_file.ggmlviz

# Export to JSON for custom analysis
./build/bin/ggml-viz --export-json trace_file.ggmlviz > analysis.json
```

## 🛠️ Troubleshooting

### Library Not Found

```
Error: GGML Visualizer shared library not found
```

**Solution**: Build the project first:
```bash
cd ggml-viz/build && make -j4
```

### No Events Recorded

```
Warning: Empty trace data
```

**Possible causes**:
- Application doesn't use GGML backend functions
- `GGML_VIZ_DISABLE=1` is set
- Function interception failed

**Solutions**:
- Check verbose logs with `GGML_VIZ_VERBOSE=1`
- Verify application uses GGML
- Try different injection method

### Permission Denied (macOS)

```
dyld: Library not loaded
```

**Solution**: 
- Disable SIP restrictions or use signed binaries
- Try `sudo` if necessary
- Use local builds instead of system applications

### Function Not Intercepted

```
Warning: No original ggml_backend_graph_compute function found
```

**Possible causes**:
- Application uses different GGML version
- Functions are statically linked
- Symbols are stripped

**Solutions**:
- Check application's GGML version compatibility
- Rebuild application with shared GGML library
- Use debug builds with symbols

## 📈 Performance Impact

The GGML Visualizer is designed for minimal overhead:

- **Without GUI**: <3% performance impact
- **With Live Viewing**: ~5-10% performance impact
- **Memory Usage**: Fixed ring buffer (65KB for events)
- **File I/O**: Batched writes every 4096 events

### Optimization Tips

1. **Reduce Event Count**: Set `GGML_VIZ_MAX_EVENTS` for short runs
2. **Disable Features**: Turn off `GGML_VIZ_MEMORY_TRACKING` if not needed
3. **Use SSD**: Store trace files on fast storage
4. **Close GUI**: Use headless mode for long runs

## 🔬 Advanced Usage

### Custom Analysis Scripts

```python
# Python script to analyze trace files
import json
import matplotlib.pyplot as plt

# Load exported JSON
with open('analysis.json') as f:
    data = json.load(f)

# Plot operation timing
ops = [event['op_type'] for event in data['events']]
times = [event['duration_ms'] for event in data['events']]

plt.scatter(range(len(times)), times, c=ops)
plt.xlabel('Operation Index')
plt.ylabel('Duration (ms)')
plt.show()
```

### Integration with Profilers

Combine with system profilers for deeper analysis:

```bash
# With perf (Linux)
perf record -g ./scripts/inject_linux.sh llama.cpp/main -m model.gguf -p "Test"

# With Instruments (macOS)  
xcrun xctrace record --template "Time Profiler" --launch ./scripts/inject_macos.sh llama.cpp/main -m model.gguf -p "Test"
```

### Continuous Integration

Add to CI pipelines for performance regression testing:

```yaml
# GitHub Actions example
- name: Profile GGML Operations
  run: |
    GGML_VIZ_OUTPUT=ci_trace.ggmlviz \
    GGML_VIZ_MAX_EVENTS=10000 \
    ./scripts/inject_linux.sh llama.cpp/main -m model.gguf -p "CI Test"
    
    # Analyze results
    ./build/bin/ggml-viz --stats ci_trace.ggmlviz > profile_results.txt
```

## 📚 Additional Resources

- [GGML Documentation](https://github.com/ggerganov/ggml)
- [llama.cpp Repository](https://github.com/ggerganov/llama.cpp)
- [whisper.cpp Repository](https://github.com/ggerganov/whisper.cpp)
- [GGML Visualizer GitHub](https://github.com/user/ggml-viz)

For more help, see the project documentation or open an issue on GitHub.