#!/bin/bash

# GGML Visualizer Performance Benchmarking Script
# Measures actual overhead of instrumentation hooks vs baseline

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
RESULTS_DIR="$PROJECT_ROOT/benchmark_results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
ITERATIONS=5
WARMUP_ITERATIONS=2

echo -e "${BLUE}🚀 GGML Visualizer Performance Benchmark${NC}"
echo "=================================================="
echo ""

# Function to print section headers
print_section() {
    echo -e "${YELLOW}$1${NC}"
    echo "$(printf '%.0s-' {1..50})"
}

# Function to run command and measure time
measure_time() {
    local description="$1"
    local command="$2"
    local output_file="$3"
    
    echo -n "Testing: $description... "
    
    # Warmup runs
    for i in $(seq 1 $WARMUP_ITERATIONS); do
        eval $command >/dev/null 2>&1 || true
    done
    
    # Actual measurement runs
    local total_time=0
    local times=()
    
    for i in $(seq 1 $ITERATIONS); do
        local start_time=$(date +%s.%N)
        eval $command >/dev/null 2>&1 || true
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l)
        times+=($duration)
        total_time=$(echo "$total_time + $duration" | bc -l)
    done
    
    local avg_time=$(echo "$total_time / $ITERATIONS" | bc -l)
    
    # Calculate standard deviation
    local sum_sq_diff=0
    for time in "${times[@]}"; do
        local diff=$(echo "$time - $avg_time" | bc -l)
        local sq_diff=$(echo "$diff * $diff" | bc -l)
        sum_sq_diff=$(echo "$sum_sq_diff + $sq_diff" | bc -l)
    done
    local variance=$(echo "$sum_sq_diff / $ITERATIONS" | bc -l)
    local std_dev=$(echo "sqrt($variance)" | bc -l)
    
    echo -e "${GREEN}Done${NC}"
    echo "  Average: $(printf "%.3f" $avg_time)s ± $(printf "%.3f" $std_dev)s"
    
    # Save results
    if [ -n "$output_file" ]; then
        echo "$description,$avg_time,$std_dev" >> "$output_file"
    fi
    
    echo "$avg_time"
}

# Function to measure memory usage
measure_memory() {
    local description="$1"
    local command="$2"
    local output_file="$3"
    
    echo -n "Memory test: $description... "
    
    # Create a script to run the command and measure memory
    local temp_script=$(mktemp)
    cat > "$temp_script" << EOF
#!/bin/bash
exec $command
EOF
    chmod +x "$temp_script"
    
    # Use time command to get memory usage (on macOS use gtime if available, otherwise ps)
    local max_memory=0
    if command -v gtime >/dev/null 2>&1; then
        # GNU time (more accurate)
        local time_output=$(gtime -f "%M" "$temp_script" 2>&1 | tail -n1)
        max_memory=$(echo "$time_output" | grep -o '[0-9]*' | tail -n1)
        # Convert from KB to MB
        max_memory=$(echo "$max_memory / 1024" | bc -l)
    else
        # Fallback: run command in background and monitor with ps
        "$temp_script" &
        local pid=$!
        local memory_samples=()
        
        while kill -0 $pid 2>/dev/null; do
            local memory=$(ps -o rss= -p $pid 2>/dev/null | tr -d ' ' || echo "0")
            if [ "$memory" -gt 0 ]; then
                memory_samples+=($memory)
            fi
            sleep 0.1
        done
        wait $pid 2>/dev/null || true
        
        # Find max memory usage and convert from KB to MB
        max_memory=0
        for mem in "${memory_samples[@]}"; do
            if (( $(echo "$mem > $max_memory" | bc -l) )); then
                max_memory=$mem
            fi
        done
        max_memory=$(echo "$max_memory / 1024" | bc -l)
    fi
    
    rm -f "$temp_script"
    
    echo -e "${GREEN}$(printf "%.1f" $max_memory) MB${NC}"
    
    # Save results
    if [ -n "$output_file" ]; then
        echo "$description,$max_memory" >> "$output_file"
    fi
    
    echo "$max_memory"
}

# Check prerequisites
print_section "Prerequisites Check"

if [ ! -f "$BUILD_DIR/bin/test_ggml_hook" ]; then
    echo -e "${RED}❌ test_ggml_hook not found. Please build the project first:${NC}"
    echo "   cd build && make -j4"
    exit 1
fi

if ! command -v bc >/dev/null 2>&1; then
    echo -e "${RED}❌ 'bc' command not found. Please install bc for calculations.${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Prerequisites met${NC}"
echo ""

# Create results directory
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TIMING_RESULTS="$RESULTS_DIR/timing_$TIMESTAMP.csv"
MEMORY_RESULTS="$RESULTS_DIR/memory_$TIMESTAMP.csv"

# Initialize CSV files
echo "Test,Average_Time_Seconds,Std_Dev_Seconds" > "$TIMING_RESULTS"
echo "Test,Max_Memory_MB" > "$MEMORY_RESULTS"

print_section "System Information"
echo "Date: $(date)"
echo "System: $(uname -a)"
echo "CPU: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || grep 'model name' /proc/cpuinfo | head -n1 | cut -d: -f2 | xargs || echo 'Unknown')"
echo "Memory: $(sysctl hw.memsize 2>/dev/null | awk '{print $2/1024/1024/1024 " GB"}' || grep MemTotal /proc/meminfo | awk '{print $2/1024/1024 " GB"}' || echo 'Unknown')"
echo "Iterations per test: $ITERATIONS (+ $WARMUP_ITERATIONS warmup)"
echo ""

cd "$BUILD_DIR"

print_section "Baseline Performance Tests"

# Test 1: Hook system with instrumentation DISABLED
echo "🔧 Testing with instrumentation DISABLED (baseline)..."
unset GGML_VIZ_OUTPUT
unset GGML_VIZ_VERBOSE
export GGML_VIZ_DISABLE=1

baseline_time=$(measure_time "Baseline (hooks disabled)" "./bin/test_ggml_hook" "$TIMING_RESULTS")
baseline_memory=$(measure_memory "Baseline (hooks disabled)" "./bin/test_ggml_hook" "$MEMORY_RESULTS")

echo ""

print_section "Instrumentation Overhead Tests"

# Test 2: Hook system with instrumentation ENABLED but no output
echo "🔧 Testing with instrumentation ENABLED (no file output)..."
unset GGML_VIZ_DISABLE
unset GGML_VIZ_OUTPUT
unset GGML_VIZ_VERBOSE

enabled_time=$(measure_time "Hooks enabled (no output)" "./bin/test_ggml_hook" "$TIMING_RESULTS")
enabled_memory=$(measure_memory "Hooks enabled (no output)" "./bin/test_ggml_hook" "$MEMORY_RESULTS")

# Test 3: Hook system with file output
echo "🔧 Testing with file output enabled..."
export GGML_VIZ_OUTPUT="benchmark_trace.ggmlviz"
rm -f "benchmark_trace.ggmlviz"

output_time=$(measure_time "Hooks with file output" "./bin/test_ggml_hook" "$TIMING_RESULTS")
output_memory=$(measure_memory "Hooks with file output" "./bin/test_ggml_hook" "$MEMORY_RESULTS")

# Test 4: Hook system with verbose logging
echo "🔧 Testing with verbose logging..."
export GGML_VIZ_VERBOSE=1
rm -f "benchmark_trace.ggmlviz"

verbose_time=$(measure_time "Hooks with verbose logging" "./bin/test_ggml_hook" "$TIMING_RESULTS")
verbose_memory=$(measure_memory "Hooks with verbose logging" "./bin/test_ggml_hook" "$MEMORY_RESULTS")

echo ""

print_section "Trace File Analysis"

if [ -f "benchmark_trace.ggmlviz" ]; then
    trace_size=$(stat -f%z "benchmark_trace.ggmlviz" 2>/dev/null || stat -c%s "benchmark_trace.ggmlviz" 2>/dev/null || echo "0")
    echo "Trace file size: $(echo "$trace_size / 1024" | bc) KB"
    
    # Test trace reader performance
    if [ -f "./bin/test_trace_reader" ]; then
        reader_time=$(measure_time "Trace file reading" "./bin/test_trace_reader benchmark_trace.ggmlviz" "$TIMING_RESULTS")
        reader_memory=$(measure_memory "Trace file reading" "./bin/test_trace_reader benchmark_trace.ggmlviz" "$MEMORY_RESULTS")
    fi
else
    echo "⚠️  No trace file generated"
fi

echo ""

print_section "Performance Analysis"

# Calculate overhead percentages
enabled_overhead=$(echo "($enabled_time - $baseline_time) / $baseline_time * 100" | bc -l)
output_overhead=$(echo "($output_time - $baseline_time) / $baseline_time * 100" | bc -l)
verbose_overhead=$(echo "($verbose_time - $baseline_time) / $baseline_time * 100" | bc -l)

memory_enabled_overhead=$(echo "($enabled_memory - $baseline_memory) / $baseline_memory * 100" | bc -l)
memory_output_overhead=$(echo "($output_memory - $baseline_memory) / $baseline_memory * 100" | bc -l)
memory_verbose_overhead=$(echo "($verbose_memory - $baseline_memory) / $baseline_memory * 100" | bc -l)

echo "📊 Performance Impact Summary:"
echo ""
echo "Timing Overhead:"
echo "  Hooks enabled (no output):   $(printf "%+.1f%%" $enabled_overhead)"
echo "  Hooks with file output:      $(printf "%+.1f%%" $output_overhead)"
echo "  Hooks with verbose logging:  $(printf "%+.1f%%" $verbose_overhead)"
echo ""
echo "Memory Overhead:"
echo "  Hooks enabled (no output):   $(printf "%+.1f%%" $memory_enabled_overhead)"
echo "  Hooks with file output:      $(printf "%+.1f%%" $memory_output_overhead)"
echo "  Hooks with verbose logging:  $(printf "%+.1f%%" $memory_verbose_overhead)"
echo ""

# Performance rating
if (( $(echo "$output_overhead < 5" | bc -l) )); then
    rating="${GREEN}Excellent${NC} (<5% overhead)"
elif (( $(echo "$output_overhead < 10" | bc -l) )); then
    rating="${YELLOW}Good${NC} (5-10% overhead)"
elif (( $(echo "$output_overhead < 20" | bc -l) )); then
    rating="${YELLOW}Acceptable${NC} (10-20% overhead)"
else
    rating="${RED}High${NC} (>20% overhead)"
fi

echo -e "Overall Performance Rating: $rating"
echo ""

print_section "Results Summary"

echo "📁 Detailed results saved to:"
echo "   Timing data: $TIMING_RESULTS"
echo "   Memory data: $MEMORY_RESULTS"
echo ""

echo "🎯 Quick Copy-Paste for Documentation:"
echo ""
echo "Performance overhead measurements ($(date +%Y-%m-%d)):"
echo "- Hooks enabled: $(printf "%.1f%%" $enabled_overhead) time, $(printf "%.1f%%" $memory_enabled_overhead) memory"
echo "- With file output: $(printf "%.1f%%" $output_overhead) time, $(printf "%.1f%%" $memory_output_overhead) memory"  
echo "- With verbose logging: $(printf "%.1f%%" $verbose_overhead) time, $(printf "%.1f%%" $memory_verbose_overhead) memory"
echo "System: $(uname -s) $(uname -m)"
echo ""

# Clean up
rm -f "benchmark_trace.ggmlviz"
unset GGML_VIZ_OUTPUT
unset GGML_VIZ_VERBOSE
unset GGML_VIZ_DISABLE

echo -e "${GREEN}✅ Benchmarking complete!${NC}"