#!/bin/bash

echo "=== Building Llama Inference Test ==="

# Navigate to build directory
cd build

# Build just the components we need
echo "Building ggml libraries..."
make ggml ggml-cpu ggml-base ggml_hook -j4

# Compile our test directly to avoid CMake linking conflicts
echo "Compiling test_llama_inference..."
g++ -std=c++17 \
    -I../src \
    -I../third_party/ggml/include \
    -DGGML_VIZ_ENABLE_HOOKS \
    -O2 \
    ../test_llama_inference.cpp \
    src/libggml_hook.a \
    third_party/ggml/src/libggml.a \
    third_party/ggml/src/libggml-cpu.a \
    third_party/ggml/src/libggml-base.a \
    -framework Accelerate \
    -lpthread \
    -o bin/test_llama_inference \
    2>/dev/null || {
        echo "Direct compilation failed, trying without hook library..."
        g++ -std=c++17 \
            -I../third_party/ggml/include \
            -DGGML_VIZ_ENABLE_HOOKS \
            -O2 \
            ../test_llama_inference.cpp \
            third_party/ggml/src/libggml.a \
            third_party/ggml/src/libggml-cpu.a \
            third_party/ggml/src/libggml-base.a \
            -framework Accelerate \
            -lpthread \
            -o bin/test_llama_inference_basic
    }

echo "✅ Build complete!"
echo "Run: ./bin/test_llama_inference or ./bin/test_llama_inference_basic"