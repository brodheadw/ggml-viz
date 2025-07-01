#!/bin/bash

echo ">ê Running GGML Visualizer Test Suite"
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "L ERROR: Please run this script from the project root directory"
    exit 1
fi

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "L ERROR: Build directory not found. Please run 'mkdir build && cd build && cmake .. && make' first"
    exit 1
fi

cd build

echo "=( Building tests..."
if ! make test_ggml_hook test_trace_reader -j4; then
    echo "L ERROR: Failed to build tests"
    exit 1
fi

echo ""
echo ">ê Running hook system test..."
if ! ./bin/test_ggml_hook; then
    echo "L FAILED: Hook system test failed"
    exit 1
fi

echo ""
echo ">ê Running trace reader test..."
if [ -f "test_trace.ggmlviz" ]; then
    if ! ./bin/test_trace_reader test_trace.ggmlviz; then
        echo "L FAILED: Trace reader test failed"
        exit 1
    fi
else
    echo "   WARNING: No trace file found, skipping trace reader test"
fi

echo ""
echo " All tests passed!"
echo ""
echo "=¡ To run with verbose output:"
echo "   GGML_VIZ_VERBOSE=1 ./scripts/run_tests.sh"