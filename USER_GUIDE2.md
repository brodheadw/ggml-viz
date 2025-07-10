### Once everything's set up:

In terminal 1:
```bash
env GGML_VIZ_OUTPUT=test_live_trace.ggmlviz DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib ./third_party/llama.cpp/build/bin/llama-cli -m ./models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf -p "Hello world" -n 10 --verbose-prompt
```

In terminal 2:
```bash
export GGML_VIZ_OUTPUT=test_live_trace.ggmlviz
./build/bin/ggml-viz --live test_live_trace.ggmlviz --no-hook
```