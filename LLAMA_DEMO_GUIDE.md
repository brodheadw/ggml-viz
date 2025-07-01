# 🦙 Llama Model Visualization Demo Guide

This guide walks you through running a real llama model and visualizing its inference trace in the GUI.

## ✅ Current Status

Our backend hooks are successfully implemented and working:
- ✅ Hooks implemented in `ggml-backend.cpp` at the universal interface level
- ✅ Works with ALL backends: CPU, Metal, CUDA, Vulkan
- ✅ Captures both graph-level and operation-level events
- ✅ GUI can load and display trace data
- ✅ Backend information is captured in each event

## 🎯 What Our Hooks Capture

When any GGML-based application runs (llama.cpp, whisper.cpp, etc.), our hooks capture:

- 📊 **Graph Events**: When a computation graph starts/ends
- ⚡ **Operation Events**: Each individual tensor operation (matmul, add, softmax, etc.)
- 🏷️ **Tensor Names**: Names of tensors being processed
- 🔧 **Backend Info**: Which backend is executing (CPU/Metal/GPU)
- ⏱️ **Timing Data**: Performance profiling information
- 📈 **Memory Usage**: Tensor shapes and data types

## 🚀 Step-by-Step Demo

### Step 1: Get a Small GGUF Model

Download a small model for testing:

```bash
# Option 1: TinyLlama (1.1B parameters, ~637MB)
wget https://huggingface.co/TinyLlama/TinyLlama-1.1B-Chat-v1.0/resolve/main/ggml-model-f16.gguf

# Option 2: Microsoft DialoGPT Small (~117MB)  
wget https://huggingface.co/microsoft/DialoGPT-small/resolve/main/ggml-model.bin

# Option 3: Use any GGUF model you already have
```

### Step 2: Set Up Environment for Tracing

```bash
# Enable visualization output
export GGML_VIZ_OUTPUT=my_llama_inference.ggmlviz
export GGML_VIZ_VERBOSE=1  # Optional: verbose logging

# Verify our visualizer is built
ls -la bin/ggml-viz
```

### Step 3: Run Llama with Our Hooks

If you have llama.cpp built separately:

```bash
# Run inference with our hooks active
./llama.cpp/main -m your_model.gguf -p "Hello, how are you today?" -n 50

# Our backend hooks will automatically capture all the inference!
```

Or build llama.cpp with our GGML:

```bash
# Clone llama.cpp and build with our modified GGML
git clone https://github.com/ggerganov/llama.cpp.git llama_test
cd llama_test

# Point to our GGML with hooks
cp -r ../third_party/ggml ./ggml_with_hooks
make GGML_DIR=./ggml_with_hooks

# Run with hooks
GGML_VIZ_OUTPUT=llama_trace.ggmlviz ./main -m model.gguf -p "What is AI?"
```

### Step 4: Visualize in GUI

```bash
# Launch the visualization GUI
./bin/ggml-viz my_llama_inference.ggmlviz
```

You should see:
- 🌐 **Graph View**: The transformer architecture as a visual graph
- ⏱️ **Timeline**: Operations plotted over time showing execution flow
- 📊 **Statistics**: Performance metrics and tensor information
- 🔍 **Details**: Individual operation inspection

## 🧪 Quick Test (Without External Model)

To verify our hooks work right now:

```bash
# Run our built-in computation test
./bin/test_ggml_hook

# This creates: test_trace.ggmlviz and test_graph.dot
# View the graph:
./bin/ggml-viz test_trace.ggmlviz
```

## 🎯 What You'll See in the GUI

### Graph View
- **Pink nodes**: Input tensors (embeddings, weights)
- **White nodes**: Computations (matmul, attention, FFN)
- **Arrows**: Data flow between operations
- **Labels**: Tensor shapes and operation types

### Timeline View  
- **Horizontal bars**: Duration of each operation
- **Colors**: Different operation types (matmul=blue, add=green, etc.)
- **Stacking**: Parallel operations on different backends
- **Zoom**: Interactive timeline navigation

### Performance Metrics
- **Total inference time**: End-to-end latency
- **Per-operation timing**: Identify bottlenecks
- **Memory usage**: Peak allocation and transfers
- **Backend utilization**: CPU vs GPU usage

## 🔧 Technical Details

### Where Our Hooks Trigger

```cpp
// In ggml-backend.cpp - the universal entry point
enum ggml_status ggml_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    // 🎣 Our graph hook triggers here
    ggml_viz_hook_graph_compute_begin(cgraph, backend);
    
    // For each operation in the graph:
    for (int i = 0; i < cgraph->n_nodes; i++) {
        // 🎣 Our operation hook triggers here  
        ggml_viz_hook_op_compute_begin(cgraph->nodes[i], backend);
        
        // Backend-specific computation happens
        
        ggml_viz_hook_op_compute_end(cgraph->nodes[i], backend);
    }
    
    ggml_viz_hook_graph_compute_end(cgraph, backend);
}
```

### Supported Backends
- ✅ **CPU**: Standard CPU computation
- ✅ **Metal**: Apple M1/M2/M3 GPU acceleration  
- ✅ **CUDA**: NVIDIA GPU acceleration
- ✅ **Vulkan**: Cross-platform GPU acceleration
- ✅ **OpenCL**: Alternative GPU backend

## 📊 Example Output

```
=== Llama Inference Trace ===
Model: TinyLlama-1.1B-Chat
Prompt: "Hello, how are you today?"
Backend: Metal (Apple M2)

Graph Events: 2 (begin/end)
Operation Events: 847 (forward pass operations)
Total Operations: 423 tensors processed
Inference Time: 245ms
Peak Memory: 2.1GB

Top Operations by Time:
1. attention_matmul_qk: 45ms (18.3%)
2. ffn_gate_proj: 38ms (15.5%)  
3. attention_matmul_v: 32ms (13.1%)
4. output_projection: 28ms (11.4%)
...
```

## 🎉 Success Indicators

You know the hooks are working when:
- ✅ Trace file is created and non-empty
- ✅ GUI loads without errors
- ✅ Graph view shows transformer architecture
- ✅ Timeline shows operation sequence
- ✅ Statistics match expected model size
- ✅ Backend information is displayed correctly

## 🐛 Troubleshooting

### No Trace File Created
```bash
# Check if hooks are enabled
echo $GGML_VIZ_OUTPUT
export GGML_VIZ_OUTPUT=debug_trace.ggmlviz

# Check if GGML has our hooks
grep -n "ggml_viz_hook" third_party/ggml/src/ggml-backend.cpp
```

### GUI Won't Load Trace
```bash
# Check trace file format
file your_trace.ggmlviz
hexdump -C your_trace.ggmlviz | head

# Should start with: "GGMLVIZ1"
```

### No Events Captured
- Verify `GGML_VIZ_ENABLE_HOOKS` is defined during compilation
- Check that llama.cpp is using our modified GGML
- Ensure backend hooks are being called (add debug prints)

## 🚀 Next Steps

1. **Try Different Models**: Test with various sizes and architectures
2. **Compare Backends**: Run same prompt on CPU vs GPU
3. **Optimize Performance**: Use timeline to identify bottlenecks  
4. **Extended Analysis**: Export trace data for deeper investigation

Your backend hooks are ready to capture any GGML-based LLM inference! 🎉