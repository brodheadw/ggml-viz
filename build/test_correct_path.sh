export GGML_VIZ_OUTPUT="real_test.ggmlviz"
export GGML_VIZ_VERBOSE=1
echo "Testing with correct llama-simple path..."
../scripts/inject_macos.sh ../test_apps/llama.cpp/build/bin/llama-simple
