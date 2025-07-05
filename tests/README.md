# GGML Visualizer Tests

This directory contains tests and development tools for the GGML Visualizer project.

## Directory Structure

### `/assets/`
Sample trace files and test data:
- `test_trace.ggmlviz` - Sample binary trace file with 60 captured events
- `test_graph.dot` - GraphViz representation of computation graph

### `/demo/`
Demonstration scripts showing system capabilities:
- `demo_metal_hooks.sh` - Shows Metal backend hook integration working

### `/integration/`
Integration test scripts for external tools:
- `demo_live_mode_with_llama.sh` - Live mode integration with llama.cpp
- `download_test_model.sh` - Downloads test model for integration tests

### Root Test Files
Core unit tests (built by CMake):
- `test_ggml_hook.cpp` - Hook system functionality tests
- `test_trace_reader.cpp` - Binary trace file reading tests
- `simple_ggml_test.cpp` - Basic GGML operations tests
- `test_ggml_functions.cpp` - GGML function integration tests

## Running Tests

```bash
# Build and run all tests
cd build
ctest

# Run specific test
./bin/test_ggml_hook

# Run with trace file
./bin/test_trace_reader tests/assets/test_trace.ggmlviz
```

## Test Coverage

- ✅ Hook system: Event capture and storage
- ✅ Trace reader: Binary file parsing
- ✅ GGML integration: Backend interception
- ✅ Metal backend: Universal hook dispatch
- ⚠️ External injection: macOS symbol resolution (known limitation)