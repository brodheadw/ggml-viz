# GGML Visualizer User Guide

A comprehensive guide to using GGML Visualizer with llama.cpp for real-time LLM performance analysis and visualization.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Installation](#installation)
3. [Basic Usage](#basic-usage)
4. [Working with llama.cpp](#working-with-llamacpp)
5. [Understanding the Interface](#understanding-the-interface)
6. [Advanced Configuration](#advanced-configuration)
7. [Performance Analysis](#performance-analysis)
8. [Troubleshooting](#troubleshooting)
9. [Tips and Best Practices](#tips-and-best-practices)

---

## Quick Start

**30-second setup for existing llama.cpp users:**

```bash
# 1. Set output file
export GGML_VIZ_OUTPUT=my_llama_trace.ggmlviz

# 2. Run your llama.cpp inference as usual
./llama.cpp/main -m your_model.gguf -p "Hello, how are you?" -n 50

# 3. Visualize the results
./ggml-viz/build/bin/ggml-viz my_llama_trace.ggmlviz
```

That's it! The visualizer will capture all GGML operations automatically.

---

## Installation

### Prerequisites

**macOS:**
```bash
brew install cmake glfw
```

**Ubuntu/Debian:**
```bash
sudo apt update && sudo apt install -y git cmake build-essential \
    libgl1-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev
```

### Build GGML Visualizer

```bash
# Clone with submodules
git clone --recursive https://github.com/your-org/ggml-visualizer.git
cd ggml-visualizer

# Build (use -DGGML_METAL=OFF on macOS if you encounter Metal shader issues)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF
make -j4

# Verify build
./bin/ggml-viz --help
```

### Verify Installation

```bash
# Run basic test
./bin/test_ggml_hook

# Check if trace file was created
ls -la *.ggmlviz

# Test GUI (should open empty dashboard)
./bin/ggml-viz
```

---

## Basic Usage

### The Two-Step Process

GGML Visualizer works in two phases:

1. **Capture Phase**: Record GGML operations during inference
2. **Analysis Phase**: Visualize and analyze the recorded data

### Command Line Interface

```bash
# Show help
./bin/ggml-viz --help

# Load a specific trace file
./bin/ggml-viz my_trace.ggmlviz

# Enable verbose logging
./bin/ggml-viz --verbose my_trace.ggmlviz

# Live mode (experimental)
./bin/ggml-viz --live --port 9000
```

### Environment Variables

**Essential Variables:**
- `GGML_VIZ_OUTPUT`: Output trace file path (required for capture)
- `GGML_VIZ_VERBOSE`: Enable detailed logging (optional)
- `GGML_VIZ_DISABLE`: Completely disable instrumentation (optional)

**Advanced Variables:**
- `GGML_VIZ_MAX_EVENTS`: Maximum events to capture (default: 10,000,000)
- `GGML_VIZ_OP_TIMING`: Enable operation timing (default: true)
- `GGML_VIZ_MEMORY_TRACKING`: Enable memory tracking (default: false)
- `GGML_VIZ_THREAD_TRACKING`: Enable thread tracking (default: false)
- `GGML_VIZ_TENSOR_NAMES`: Capture tensor names (default: true)

---

## Working with llama.cpp

### Option 1: Use with Existing llama.cpp Installation

This is the easiest method - works with any existing llama.cpp installation:

```bash
# Set up environment
export GGML_VIZ_OUTPUT=llama_inference.ggmlviz
export GGML_VIZ_VERBOSE=1

# Run llama.cpp normally (replace paths with your actual paths)
cd /path/to/your/llama.cpp
./main -m models/your_model.gguf \
       -p "Explain quantum computing in simple terms" \
       -n 100 \
       --temp 0.7

# Visualize results
cd /path/to/ggml-visualizer/build
./bin/ggml-viz llama_inference.ggmlviz
```

### Option 2: Build llama.cpp with GGML-Viz's Modified GGML

For maximum compatibility and features:

```bash
# Clone llama.cpp
git clone https://github.com/ggerganov/llama.cpp.git llama_with_viz
cd llama_with_viz

# Replace GGML with our instrumented version
rm -rf ggml
cp -r /path/to/ggml-visualizer/third_party/ggml ./ggml

# Build llama.cpp
make -j4

# Test with instrumentation
export GGML_VIZ_OUTPUT=llama_trace.ggmlviz
./main -m your_model.gguf -p "Hello world" -n 10

# Visualize
/path/to/ggml-visualizer/build/bin/ggml-viz llama_trace.ggmlviz
```

### Model Recommendations for Testing

**Small models for initial testing:**
- **TinyLlama-1.1B**: Fast inference, good for testing
- **Phi-2 (2.7B)**: Excellent quality-to-size ratio
- **CodeLlama-7B-Instruct**: Great for code-related tasks

**Download example:**
```bash
# Using Hugging Face Hub
pip install huggingface_hub
huggingface-cli download microsoft/Phi-3-mini-4k-instruct-gguf \
    Phi-3-mini-4k-instruct-q4.gguf --local-dir models/
```

### Common llama.cpp Integration Patterns

**Text Generation:**
```bash
export GGML_VIZ_OUTPUT=text_generation.ggmlviz
./main -m model.gguf -p "Write a haiku about programming:" -n 50
```

**Interactive Chat:**
```bash
export GGML_VIZ_OUTPUT=chat_session.ggmlviz
./main -m model.gguf -p "System: You are a helpful assistant.\nUser: Hello!\nAssistant:" -i
```

**Batch Processing:**
```bash
export GGML_VIZ_OUTPUT=batch_inference.ggmlviz
for prompt in "Hello" "Goodbye" "Thank you"; do
    ./main -m model.gguf -p "$prompt" -n 20
done
```

---

## Understanding the Interface

### Main Dashboard Components

When you open a trace file, you'll see several key areas:

#### 1. Graph View
- **Purpose**: Visual representation of the computation graph
- **What to look for**: 
  - Model architecture (attention, feed-forward layers)
  - Tensor shapes and data types
  - Operation sequence and dependencies

#### 2. Timeline View
- **Purpose**: Chronological view of operations during inference
- **What to look for**:
  - Operation execution order
  - Time spent per operation
  - Backend utilization (CPU/Metal/CUDA)
  - Bottlenecks and performance hotspots

#### 3. Statistics Panel
- **Purpose**: Numerical summary of the inference session
- **Key metrics**:
  - Total inference time
  - Number of operations executed
  - Memory usage patterns
  - Backend distribution

#### 4. Operation Details
- **Purpose**: Detailed information about individual operations
- **Information shown**:
  - Operation type (matmul, add, softmax, etc.)
  - Input/output tensor shapes
  - Execution time
  - Backend used

### What the Data Tells You

**Graph Structure Insights:**
- **Transformer blocks**: Repeated patterns indicate layers
- **Attention mechanisms**: Complex matmul patterns
- **Feed-forward networks**: Linear transformations
- **Embeddings**: Input/output token processing

**Performance Insights:**
- **Hot operations**: Operations taking the most time
- **Backend efficiency**: How well different backends perform
- **Memory patterns**: Allocation and deallocation timing
- **Parallelization**: Thread utilization across operations

---

## Advanced Configuration

### Custom Configuration Files

Create a `config.json` file for repeated use:

```json
{
  "output_file": "default_trace.ggmlviz",
  "max_events": 5000000,
  "enable_timing": true,
  "enable_memory_tracking": true,
  "enable_thread_tracking": false,
  "capture_tensor_names": true,
  "verbose_logging": false,
  "backends": {
    "cpu": true,
    "metal": true,
    "cuda": true,
    "vulkan": true
  }
}
```

Load with:
```bash
./bin/ggml-viz --config config.json my_trace.ggmlviz
```

### Performance Optimization

**For Large Models (>7B parameters):**
```bash
# Limit event capture to prevent memory issues
export GGML_VIZ_MAX_EVENTS=1000000
export GGML_VIZ_MEMORY_TRACKING=false
export GGML_VIZ_THREAD_TRACKING=false
```

**For Performance Analysis:**
```bash
# Enable all tracking for detailed analysis
export GGML_VIZ_MAX_EVENTS=10000000
export GGML_VIZ_OP_TIMING=true
export GGML_VIZ_MEMORY_TRACKING=true
export GGML_VIZ_THREAD_TRACKING=true
export GGML_VIZ_VERBOSE=1
```

**For Production Monitoring:**
```bash
# Minimal overhead capture
export GGML_VIZ_MAX_EVENTS=100000
export GGML_VIZ_OP_TIMING=false
export GGML_VIZ_MEMORY_TRACKING=false
export GGML_VIZ_TENSOR_NAMES=false
```

### Multi-Backend Analysis

Compare performance across different backends:

```bash
# CPU-only run
export GGML_VIZ_OUTPUT=cpu_run.ggmlviz
CUDA_VISIBLE_DEVICES="" ./main -m model.gguf -p "Test" -n 50

# Metal run (macOS)
export GGML_VIZ_OUTPUT=metal_run.ggmlviz
./main -m model.gguf -p "Test" -n 50

# Compare in visualizer
./bin/ggml-viz cpu_run.ggmlviz
./bin/ggml-viz metal_run.ggmlviz
```

---

## Performance Analysis

### Key Metrics to Monitor

#### 1. Inference Speed
- **Tokens per second**: Overall generation speed
- **Time per token**: Latency for each generated token
- **First token latency**: Time to start generation

#### 2. Operation Efficiency
- **Matrix multiplication time**: Usually the bottleneck
- **Attention computation**: Self-attention overhead
- **Activation functions**: ReLU, GELU, SiLU performance
- **Memory operations**: Data movement costs

#### 3. Resource Utilization
- **Backend distribution**: CPU vs GPU utilization
- **Memory usage**: Peak and sustained memory
- **Thread efficiency**: Parallel execution patterns

### Performance Optimization Strategies

**Based on Timeline Analysis:**

1. **Identify Bottlenecks**:
   ```
   Look for operations taking >10% of total time
   Focus on matmul operations first
   Check for memory transfer overhead
   ```

2. **Backend Optimization**:
   ```
   Compare CPU vs Metal/CUDA performance
   Consider mixed-precision inference
   Evaluate quantization impact
   ```

3. **Model-Specific Tuning**:
   ```
   Analyze attention pattern efficiency
   Look for redundant operations
   Consider architectural modifications
   ```

### Benchmarking Workflow

```bash
# Create a standardized benchmark
echo "Benchmarking model performance..."

# Short generation for latency
export GGML_VIZ_OUTPUT=latency_test.ggmlviz
./main -m model.gguf -p "Hello" -n 1

# Medium generation for throughput  
export GGML_VIZ_OUTPUT=throughput_test.ggmlviz
./main -m model.gguf -p "Write a story about:" -n 100

# Long generation for stability
export GGML_VIZ_OUTPUT=stability_test.ggmlviz
./main -m model.gguf -p "Explain machine learning in detail:" -n 500

# Analyze all three
for trace in latency_test throughput_test stability_test; do
    echo "Analyzing $trace..."
    ./bin/ggml-viz ${trace}.ggmlviz
done
```

---

## Troubleshooting

### Common Issues and Solutions

#### 1. No Trace File Generated

**Problem**: GGML_VIZ_OUTPUT is set but no .ggmlviz file appears

**Solutions**:
```bash
# Check if environment variable is properly set
echo $GGML_VIZ_OUTPUT

# Ensure directory exists and is writable
mkdir -p $(dirname $GGML_VIZ_OUTPUT)
touch $GGML_VIZ_OUTPUT  # Test write permissions

# Enable verbose logging to debug
export GGML_VIZ_VERBOSE=1
```

#### 2. Empty Trace Files

**Problem**: Trace file exists but contains no events

**Possible causes**:
- GGML hooks not properly integrated
- Application not using GGML backend system
- Instrumentation disabled

**Solutions**:
```bash
# Test with known working application
./bin/test_ggml_hook

# Check if hooks are compiled in
export GGML_VIZ_VERBOSE=1
# Look for hook initialization messages
```

#### 3. GUI Won't Load Trace Files

**Problem**: Visualizer opens but can't load trace files

**Solutions**:
```bash
# Verify file format
file your_trace.ggmlviz

# Check file permissions
ls -la your_trace.ggmlviz

# Test with known good trace
./bin/test_ggml_hook  # Generates test_trace.ggmlviz
./bin/ggml-viz test_trace.ggmlviz
```

#### 4. Performance Overhead Too High

**Problem**: Inference becomes significantly slower with instrumentation

**Solutions**:
```bash
# Reduce event capture
export GGML_VIZ_MAX_EVENTS=100000

# Disable expensive tracking
export GGML_VIZ_MEMORY_TRACKING=false
export GGML_VIZ_THREAD_TRACKING=false
export GGML_VIZ_TENSOR_NAMES=false

# Or disable completely for production
export GGML_VIZ_DISABLE=1
```

#### 5. Build Issues

**Problem**: Compilation errors during build

**Common solutions**:
```bash
# For Metal shader issues on macOS
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF

# For missing dependencies
brew install cmake glfw  # macOS
sudo apt install cmake libgl1-mesa-dev libglfw3-dev  # Ubuntu

# Clean rebuild
rm -rf build/*
cmake .. -DCMAKE_BUILD_TYPE=Release
make clean && make -j4
```

### Debug Information

**Enable maximum debugging**:
```bash
export GGML_VIZ_VERBOSE=1
export GGML_VIZ_OUTPUT=debug_trace.ggmlviz
./your_llama_command 2>&1 | tee debug.log
```

**Check system compatibility**:
```bash
# Verify GGML backend support
./bin/ggml-viz --version

# Check available backends
./bin/test_ggml_hook  # Look for backend information in output
```

---

## Tips and Best Practices

### Development Workflow

1. **Start Small**: Use small models (1-3B parameters) for initial development
2. **Iterative Analysis**: Capture short runs first, then gradually increase length
3. **Comparative Analysis**: Always compare before/after when making changes
4. **Document Findings**: Keep notes on performance patterns you discover

### Production Usage

1. **Selective Instrumentation**: Only enable when needed
2. **Automated Analysis**: Script trace capture for CI/CD pipelines
3. **Performance Budgets**: Set acceptable overhead limits
4. **Regular Monitoring**: Capture traces for performance regression detection

### Model-Specific Tips

**For Large Language Models:**
- Focus on attention and feed-forward layer performance
- Monitor memory usage during long sequences
- Analyze token generation patterns

**For Code Models:**
- Pay attention to special token processing
- Monitor performance with different code structures
- Analyze tokenization efficiency

**For Chat Models:**
- Capture multi-turn conversations
- Monitor context window utilization
- Analyze system prompt impact

### Advanced Techniques

**Comparative Benchmarking:**
```bash
# A/B test different model versions
for model in model_v1.gguf model_v2.gguf; do
    export GGML_VIZ_OUTPUT=${model%.*}_trace.ggmlviz
    ./main -m $model -p "Standard test prompt" -n 100
done
```

**Performance Regression Testing:**
```bash
# Automated performance monitoring
#!/bin/bash
BASELINE_TRACE="baseline_performance.ggmlviz"
CURRENT_TRACE="current_performance.ggmlviz"

# Run standardized test
export GGML_VIZ_OUTPUT=$CURRENT_TRACE
./main -m model.gguf -p "Standardized test prompt" -n 50

# Compare with baseline (manual analysis for now)
echo "Compare $BASELINE_TRACE with $CURRENT_TRACE in visualizer"
./bin/ggml-viz $CURRENT_TRACE
```

**Custom Analysis Scripts:**
```bash
# Extract key metrics from trace files
#!/bin/bash
for trace in *.ggmlviz; do
    echo "Analysis for $trace:"
    ./bin/test_trace_reader $trace | grep -E "(Event count|Total duration)"
done
```

---

## Next Steps

Now that you have GGML Visualizer set up and understand the basics:

1. **Try with Your Models**: Start with your existing llama.cpp models
2. **Experiment with Settings**: Test different configuration options
3. **Analyze Performance**: Identify bottlenecks in your specific use cases
4. **Optimize Based on Data**: Make informed decisions about model and hardware choices
5. **Share Findings**: Contribute insights back to the community

For advanced features and development, see:
- `TODO.md` - Planned features and development roadmap
- `CLAUDE.md` - Development guidelines and architecture details
- GitHub Issues - Report bugs and request features

Happy analyzing! 🚀