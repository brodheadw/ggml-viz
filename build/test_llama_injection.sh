export GGML_VIZ_OUTPUT="llama_live_test.ggmlviz"
export GGML_VIZ_VERBOSE=1
../scripts/inject_macos.sh ../test_apps/llama.cpp/build/bin/llama-simple --help
