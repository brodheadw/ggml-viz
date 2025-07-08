#!/bin/bash

# Simple GGML Visualizer Performance Test
# Quick overhead measurement without complex calculations

echo "🚀 GGML Visualizer Quick Performance Test"
echo "=========================================="
echo ""

cd build || exit 1

echo "System: $(uname -a)"
echo "CPU: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo 'Unknown')"
echo "Date: $(date)"
echo ""

# Test 1: Baseline with no output file (should be minimal overhead)
echo "📊 Test 1: Hooks enabled, no file output"
unset GGML_VIZ_OUTPUT
unset GGML_VIZ_VERBOSE  
unset GGML_VIZ_DISABLE

echo -n "Running... "
time1=$(bash -c 'time ./bin/test_ggml_hook >/dev/null 2>&1' 2>&1 | grep real | awk '{print $2}')
echo "Done - Time: $time1"
echo ""

# Test 2: With file output
echo "📊 Test 2: Hooks enabled, with file output"
export GGML_VIZ_OUTPUT="perf_test.ggmlviz"
rm -f perf_test.ggmlviz

echo -n "Running... "
time2=$(bash -c 'time ./bin/test_ggml_hook >/dev/null 2>&1' 2>&1 | grep real | awk '{print $2}')
echo "Done - Time: $time2"

if [ -f "perf_test.ggmlviz" ]; then
    size=$(wc -c < perf_test.ggmlviz)
    echo "Trace file size: $size bytes"
else
    echo "⚠️  No trace file generated"
fi
echo ""

# Test 3: With verbose logging
echo "📊 Test 3: Hooks enabled, with verbose logging"
export GGML_VIZ_VERBOSE=1
rm -f perf_test.ggmlviz

echo -n "Running... "
time3=$(bash -c 'time ./bin/test_ggml_hook >/dev/null 2>&1' 2>&1 | grep real | awk '{print $2}')
echo "Done - Time: $time3"
echo ""

# Test 4: Multiple runs for stability
echo "📊 Test 4: Multiple runs (5x) for timing stability"
export GGML_VIZ_OUTPUT="perf_test.ggmlviz"
unset GGML_VIZ_VERBOSE

echo "Times:"
for i in {1..5}; do
    rm -f perf_test.ggmlviz
    time_run=$(bash -c 'time ./bin/test_ggml_hook >/dev/null 2>&1' 2>&1 | grep real | awk '{print $2}')
    echo "  Run $i: $time_run"
done
echo ""

# Memory test
echo "📊 Test 5: Memory usage check"
echo -n "Running with memory monitoring... "
rm -f perf_test.ggmlviz

# Run in background and monitor memory
./bin/test_ggml_hook >/dev/null 2>&1 &
pid=$!
max_memory=0

while kill -0 $pid 2>/dev/null; do
    if command -v pgrep >/dev/null; then
        memory=$(ps -o rss= -p $pid 2>/dev/null | tr -d ' ' || echo "0")
        if [ "$memory" -gt "$max_memory" ]; then
            max_memory=$memory
        fi
    fi
    sleep 0.05
done
wait $pid

if [ "$max_memory" -gt 0 ]; then
    memory_mb=$((max_memory / 1024))
    echo "Peak memory: ${memory_mb} MB"
else
    echo "Memory monitoring unavailable"
fi
echo ""

# Summary
echo "🎯 Quick Performance Summary:"
echo "=============================="
echo "No file output:     $time1"
echo "With file output:   $time2"  
echo "With verbose:       $time3"
echo ""
echo "✅ Basic performance data collected!"
echo ""
echo "💡 For detailed analysis, run: ./scripts/benchmark.sh"

# Cleanup
rm -f perf_test.ggmlviz
unset GGML_VIZ_OUTPUT
unset GGML_VIZ_VERBOSE