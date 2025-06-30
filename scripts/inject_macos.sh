#!/bin/bash
# macOS GGML Visualizer Injection Script
# Usage: ./inject_macos.sh <command> [args...]

set -e

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Check if the shared library exists
# Try versioned name first, then fallback to unversioned
if [[ -f "$BUILD_DIR/src/libggml_viz_hook.0.1.dylib" ]]; then
    SHARED_LIB="$BUILD_DIR/src/libggml_viz_hook.0.1.dylib"
elif [[ -f "$BUILD_DIR/src/libggml_viz_hook.dylib" ]]; then
    SHARED_LIB="$BUILD_DIR/src/libggml_viz_hook.dylib"
else
    echo "❌ Error: Could not find GGML visualization hook library"
    echo "   Looked for: $BUILD_DIR/src/libggml_viz_hook.0.1.dylib"
    echo "   Looked for: $BUILD_DIR/src/libggml_viz_hook.dylib"
    exit 1
fi
if [[ ! -f "$SHARED_LIB" ]]; then
    echo "Error: GGML Visualizer shared library not found at $SHARED_LIB"
    echo "Please build the project first:"
    echo "  cd $PROJECT_ROOT && mkdir -p build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release && make -j4"
    exit 1
fi

# Check if output file is specified
if [[ -z "$GGML_VIZ_OUTPUT" ]]; then
    # Default output filename with timestamp
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    export GGML_VIZ_OUTPUT="ggml_trace_${TIMESTAMP}.ggmlviz"
    echo "GGML_VIZ_OUTPUT not set, using: $GGML_VIZ_OUTPUT"
fi

# Check if we have a command to run
if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <command> [args...]"
    echo ""
    echo "Examples:"
    echo "  $0 llama.cpp/main -m model.gguf -p 'Hello, world!'"
    echo "  $0 whisper.cpp/main -m model.bin -f audio.wav"
    echo ""
    echo "Environment variables:"
    echo "  GGML_VIZ_OUTPUT          - Output trace file (default: auto-generated)"
    echo "  GGML_VIZ_VERBOSE         - Enable verbose logging (1/0)"
    echo "  GGML_VIZ_OP_TIMING       - Enable operation timing (default: 1)"
    echo "  GGML_VIZ_MEMORY_TRACKING - Enable memory tracking (default: 0)"
    echo "  GGML_VIZ_TENSOR_NAMES    - Enable tensor names (default: 1)"
    echo "  GGML_VIZ_MAX_EVENTS      - Maximum events to record (default: 10000000)"
    echo "  GGML_VIZ_DISABLE         - Disable instrumentation (1/0)"
    exit 1
fi

# Set default environment variables if not specified
export GGML_VIZ_VERBOSE=${GGML_VIZ_VERBOSE:-0}
export GGML_VIZ_OP_TIMING=${GGML_VIZ_OP_TIMING:-1}
export GGML_VIZ_MEMORY_TRACKING=${GGML_VIZ_MEMORY_TRACKING:-0}
export GGML_VIZ_TENSOR_NAMES=${GGML_VIZ_TENSOR_NAMES:-1}
export GGML_VIZ_MAX_EVENTS=${GGML_VIZ_MAX_EVENTS:-10000000}

echo "=============================================="
echo "🚀 GGML Visualizer Injection (macOS)"
echo "=============================================="
echo "Library: $SHARED_LIB"
echo "Output:  $GGML_VIZ_OUTPUT"
echo "Command: $*"
echo ""

# Show configuration
if [[ "$GGML_VIZ_VERBOSE" == "1" ]]; then
    echo "Configuration:"
    echo "  Op timing: $GGML_VIZ_OP_TIMING"
    echo "  Memory tracking: $GGML_VIZ_MEMORY_TRACKING" 
    echo "  Tensor names: $GGML_VIZ_TENSOR_NAMES"
    echo "  Max events: $GGML_VIZ_MAX_EVENTS"
    echo ""
fi

# Run the command with library injection
echo "Starting application with GGML instrumentation..."
echo ""

# Set up DYLD environment for macOS
export DYLD_INSERT_LIBRARIES="$SHARED_LIB"
export DYLD_FORCE_FLAT_NAMESPACE=1

# Execute the command
exec "$@"