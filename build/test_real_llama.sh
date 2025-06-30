export GGML_VIZ_OUTPUT="llama_real_test.ggmlviz"
export GGML_VIZ_VERBOSE=1
echo "Testing with llama-simple (this will fail without model but should generate some GGML calls)..."
../scripts/inject_macos.sh ../test_apps/llama.cpp/build/bin/llama-simple -m /nonexistent/model.gguf
