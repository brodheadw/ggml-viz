#\!/bin/bash

# Demo script showing live GGML visualization with llama.cpp
# This demonstrates the communication bridge between GUI and external llama processes

echo "🦙 GGML Visualizer + llama.cpp Live Demo"
echo "========================================"
echo ""

# Set the shared trace file for communication
export GGML_VIZ_OUTPUT="demo_live_trace.ggmlviz"

echo "📁 Using trace file: $GGML_VIZ_OUTPUT"
echo ""

# Check if model exists
MODEL_PATH="models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
if [ \! -f "$MODEL_PATH" ]; then
    echo "❌ Model not found: $MODEL_PATH"
    echo "Run ./download_test_model.sh first to download a test model"
    exit 1
fi

echo "✅ Model found: $MODEL_PATH"
echo ""

# Clean up any existing trace file
rm -f "$GGML_VIZ_OUTPUT"

echo "🚀 Starting GGML Visualizer in live mode..."
echo "   The GUI will monitor file: $GGML_VIZ_OUTPUT"
echo "   Live events from llama.cpp will appear in real-time!"
echo ""
echo "Instructions:"
echo "1. First, the visualizer web server will start"
echo "2. Then run llama.cpp inference with hook injection:"
echo "   DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \\"
echo "   ./third_party/llama.cpp/build/bin/llama-cli -m $MODEL_PATH -p 'Hello world' -n 50"
echo "3. Watch the events in your browser at http://localhost:8080"
echo ""

# Start the visualizer in web mode for easier debugging
./build/bin/ggml-viz --web --port 8080 --verbose &

echo ""
echo "🌐 Web Dashboard: http://localhost:8080"
echo "📡 Event Stream:  http://localhost:8080/events" 
echo "📊 Status API:    http://localhost:8080/status"
echo ""
echo "Now run inference with hook injection:"
echo ""
echo "GGML_VIZ_OUTPUT=$GGML_VIZ_OUTPUT \\"
echo "GGML_VIZ_VERBOSE=1 \\"
echo "DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \\"
echo "./third_party/llama.cpp/build/bin/llama-cli -m $MODEL_PATH -p 'Hello world' -n 10"