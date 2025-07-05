#\!/bin/bash

# Download a small test model for GGML visualization
echo "Downloading TinyLlama-1.1B-Chat (Q4_K_M quantized)..."
echo "This is a small 669MB model perfect for testing."

# Create models directory
mkdir -p models

# Download TinyLlama model
wget -O models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
  "https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"

echo "Model downloaded to: models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
echo ""
echo "To test with GGML visualizer:"
echo "1. Start GUI: export GGML_VIZ_OUTPUT=live_trace.ggmlviz && ./build/bin/ggml-viz --live"
echo "2. Run inference: export GGML_VIZ_OUTPUT=live_trace.ggmlviz && ./third_party/llama.cpp/build/bin/llama-cli -m models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf -p 'Hello world' -n 50"
EOF < /dev/null