#!/bin/bash

# test_loading.sh - Test if our library is being loaded correctly

echo "Building hook library..."
cd build
make -j8 ggml_hook

echo "Checking if library was built..."
if [ ! -f "src/libggml_hook.dylib" ]; then
    echo "ERROR: libggml_hook.dylib not found!"
    echo "Available libraries in src/:"
    ls -la src/lib*
    exit 1
fi

echo "Library file info:"
ls -la src/libggml_hook.dylib
file src/libggml_hook.dylib

echo "Checking exported symbols..."
nm -D src/libggml_hook.dylib | grep -E "(ggml_|malloc)" | head -10

echo ""
echo "=== Testing library injection with simple command ==="

# Test 1: Simple command to see if library loads
echo "Test 1: Library loading with 'echo' command"
DYLD_INSERT_LIBRARIES=./src/libggml_hook.dylib \
DYLD_FORCE_FLAT_NAMESPACE=1 \
echo "Hello from injected library"

echo ""
echo "Test 2: Library loading with llama-cli --help"
DYLD_INSERT_LIBRARIES=./src/libggml_hook.dylib \
DYLD_FORCE_FLAT_NAMESPACE=1 \
./third_party/llama.cpp/llama-cli --help | head -5

echo ""
echo "Test 3: Actual model inference with small prompt"
echo "Forcing CPU-only execution..."

export GGML_NO_METAL=1
export GGML_NO_ACCELERATE=1
export OMP_NUM_THREADS=1

DYLD_INSERT_LIBRARIES=./src/libggml_hook.dylib \
DYLD_FORCE_FLAT_NAMESPACE=1 \
./third_party/llama.cpp/llama-cli \
    --model ../models/llama-3.2-1b-instruct-q4_0.gguf \
    --prompt "Hi" \
    --n-predict 3 \
    --threads 1 \
    --simple-io \
    2>&1 | tee injection_test.log

echo ""
echo "=== Results ==="
echo "Log file size: $(wc -l < injection_test.log) lines"
echo "Trace file: $(ls -la ggml_trace.bin 2>/dev/null || echo 'NOT FOUND')"
echo ""
echo "Searching for our debug messages in log:"
grep -E "(GGML_VIZ|DEBUG.*ggml)" injection_test.log || echo "No GGML_VIZ messages found"

echo ""
echo "If you see 'GGML_VIZ LIBRARY LOADED' above, the injection is working."
echo "If not, there's a library loading issue to fix first."