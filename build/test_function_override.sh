export GGML_VIZ_OUTPUT="function_override_test.ggmlviz"
export GGML_VIZ_VERBOSE=1

# Test with llama-simple but provide a simple prompt to try to trigger computation
echo "Testing function override with actual computation attempt..."
echo "Note: This will fail due to missing model, but may call GGML functions during initialization"

../scripts/inject_macos.sh ../test_apps/llama.cpp/build/bin/llama-simple -m /tmp/fake.gguf -p "test" --verbose
