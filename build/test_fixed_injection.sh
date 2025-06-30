export GGML_VIZ_OUTPUT="FIXED_test.ggmlviz"
export GGML_VIZ_VERBOSE=1
echo "Testing with FIXED injection script..."
../scripts/inject_macos.sh ../test_apps/llama.cpp/build/bin/llama-simple --help
