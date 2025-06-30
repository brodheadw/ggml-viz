#!/bin/bash

echo "=== 🔍 Debugging Function Override ==="

# Enhanced debug version
export GGML_VIZ_OUTPUT="override_debug.ggmlviz"
export GGML_VIZ_VERBOSE=1
export DYLD_PRINT_LIBRARIES=1  # Show what libraries are loaded
export DYLD_PRINT_BINDINGS=1   # Show symbol binding (this might be very verbose)

echo ""
echo "1️⃣ Testing with enhanced debug output..."
echo "   This will show library loading and symbol resolution..."

# Run with a simple command first
echo "Testing with simple --help..."
../scripts/inject_macos.sh ../test_apps/llama.cpp/build/bin/llama-simple --help 2>&1 | grep -E "(DEBUG|GGML|dyld|Intercepted)" | head -20

echo ""
echo "2️⃣ Checking if our override functions are present in our library..."
nm -D ../src/libggml_viz_hook.dylib 2>/dev/null | grep ggml || nm ../src/libggml_viz_hook.dylib 2>/dev/null | grep ggml

echo ""
echo "3️⃣ Checking if GGML functions exist in the target libraries..."
echo "In libggml.dylib:"
nm ../test_apps/llama.cpp/build/bin/libggml.dylib 2>/dev/null | grep -E "ggml_graph_compute|ggml_backend_graph_compute" | head -5

echo "In libggml-cpu.dylib:"  
nm ../test_apps/llama.cpp/build/bin/libggml-cpu.dylib 2>/dev/null | grep -E "ggml_graph_compute|ggml_backend_graph_compute" | head -5

echo ""
echo "🔍 Analysis complete!"