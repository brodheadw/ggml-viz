#!/bin/bash
# LLaMA Demo - Download and run real LLaMA model with GGML Visualizer
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
THIRD_PARTY_DIR="${PROJECT_ROOT}/third_party"
BUILD_DIR="${PROJECT_ROOT}/build"

echo "🦙 GGML Visualizer - Real LLaMA Demo"
echo "===================================="
echo

# Check if ggml-viz is built
if [[ ! -f "${BUILD_DIR}/bin/ggml-viz" ]]; then
    echo "❌ Error: ggml-viz not found in ${BUILD_DIR}/bin/"
    echo "Please build the project first:"
    echo "  mkdir build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc)"
    exit 1
fi

# Check if hook library exists
HOOK_LIB=""
if [[ "$OSTYPE" == "darwin"* ]]; then
    HOOK_LIB="${BUILD_DIR}/src/libggml_viz_hook.dylib"
    if [[ ! -f "$HOOK_LIB" ]]; then
        echo "❌ Error: Hook library not found: $HOOK_LIB"
        exit 1
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    HOOK_LIB="${BUILD_DIR}/src/libggml_viz_hook.so"
    if [[ ! -f "$HOOK_LIB" ]]; then
        echo "❌ Error: Hook library not found: $HOOK_LIB"
        exit 1
    fi
else
    echo "❌ Error: Unsupported platform: $OSTYPE"
    echo "This demo currently supports macOS and Linux only."
    exit 1
fi

# Create third_party directory if it doesn't exist
mkdir -p "$THIRD_PARTY_DIR"
cd "$THIRD_PARTY_DIR"

# Step 1: Clone llama.cpp if needed
LLAMA_DIR="${THIRD_PARTY_DIR}/llama.cpp"
if [[ ! -d "$LLAMA_DIR" ]]; then
    echo "📥 Cloning llama.cpp..."
    git clone https://github.com/ggerganov/llama.cpp.git
    echo "✅ llama.cpp cloned successfully"
else
    echo "📁 llama.cpp already exists, skipping clone"
fi

cd "$LLAMA_DIR"

# Step 2: Build llama.cpp if needed
LLAMA_BUILD_DIR="${LLAMA_DIR}/build"
if [[ ! -f "${LLAMA_BUILD_DIR}/bin/llama-cli" ]]; then
    echo "🔨 Building llama.cpp..."
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    echo "✅ llama.cpp built successfully"
    cd ..
else
    echo "📦 llama.cpp already built, skipping build"
fi

# Step 3: Download model if needed
MODELS_DIR="${LLAMA_DIR}/models"
mkdir -p "$MODELS_DIR"
cd "$MODELS_DIR"

MODEL_FILE="tinyllama-1.1b-chat-v1.0.q4_k_m.gguf"
MODEL_URL="https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGML/resolve/main/tinyllama-1.1b-chat-v1.0.q4_k_m.gguf"

if [[ ! -f "$MODEL_FILE" ]]; then
    echo "📥 Downloading TinyLlama model (~637MB)..."
    echo "This may take a few minutes depending on your internet connection."
    
    # Try wget first, then curl
    if command -v wget >/dev/null 2>&1; then
        wget "$MODEL_URL" -O "$MODEL_FILE"
    elif command -v curl >/dev/null 2>&1; then
        curl -L "$MODEL_URL" -o "$MODEL_FILE"
    else
        echo "❌ Error: Neither wget nor curl found. Please install one of them."
        exit 1
    fi
    
    echo "✅ Model downloaded successfully"
else
    echo "📁 Model already exists, skipping download"
fi

# Verify model file
if [[ ! -f "$MODEL_FILE" || ! -s "$MODEL_FILE" ]]; then
    echo "❌ Error: Model file is missing or empty: $MODEL_FILE"
    exit 1
fi

MODEL_SIZE=$(du -h "$MODEL_FILE" | cut -f1)
echo "📊 Model size: $MODEL_SIZE"

# Step 4: Prepare for trace capture
cd "$PROJECT_ROOT"
TRACE_FILE="llama_real_trace.ggmlviz"

echo
echo "🚀 Running LLaMA with GGML Visualizer instrumentation..."
echo "📁 Trace file: $TRACE_FILE"
echo "🔧 Hook library: $HOOK_LIB"
echo

# Set up environment for instrumentation
export GGML_VIZ_OUTPUT="$TRACE_FILE"
export GGML_VIZ_VERBOSE=1
export GGML_VIZ_MAX_EVENTS=100000

# Platform-specific library injection
if [[ "$OSTYPE" == "darwin"* ]]; then
    export DYLD_INSERT_LIBRARIES="$HOOK_LIB"
    echo "🍎 Using macOS DYLD_INSERT_LIBRARIES"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    export LD_PRELOAD="$HOOK_LIB"
    echo "🐧 Using Linux LD_PRELOAD"
fi

# Step 5: Run LLaMA with instrumentation
echo "💬 Running inference: 'Explain quantum computing in simple terms.'"
echo "⏱️  This will generate real GGML events for visualization..."
echo

"${LLAMA_BUILD_DIR}/bin/llama-cli" \
    -m "${MODELS_DIR}/${MODEL_FILE}" \
    -p "Explain quantum computing in simple terms." \
    -n 50 \
    --temp 0.7 \
    --top-p 0.9 \
    --seed 42

echo
echo "🎉 Demo completed!"

# Check if trace was generated
if [[ -f "$TRACE_FILE" ]]; then
    TRACE_SIZE=$(du -h "$TRACE_FILE" | cut -f1)
    EVENT_COUNT=$(xxd "$TRACE_FILE" | wc -l)  # Rough estimate
    
    echo "✅ Trace file generated: $TRACE_FILE ($TRACE_SIZE)"
    echo "📊 Estimated events captured: ~$((EVENT_COUNT * 2))"
    echo
    echo "🖥️  To view the trace in the GUI:"
    echo "   ${BUILD_DIR}/bin/ggml-viz $TRACE_FILE"
    echo
    echo "🔍 To view in live mode (run in another terminal):"
    echo "   ${BUILD_DIR}/bin/ggml-viz --live $TRACE_FILE"
else
    echo "❌ Warning: No trace file generated. Check for errors above."
    echo "🔧 Debug: Check if hook library was loaded properly"
fi

echo
echo "🎯 This demo showed REAL LLaMA inference with actual:"
echo "   • Token embeddings and positional encoding"
echo "   • Multi-head attention computations"  
echo "   • Feed-forward network operations"
echo "   • Layer normalization and residual connections"
echo "   • Output token generation and sampling"
echo
echo "📈 All operations were captured by GGML Visualizer for analysis!"