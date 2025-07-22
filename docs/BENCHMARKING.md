# GGML Visualizer Benchmarking Guide

This document describes the performance benchmarking infrastructure for GGML Visualizer and provides guidance on measuring instrumentation overhead.

## Quick Start

```bash
# Simple performance test (recommended for most users)
./scripts/simple_benchmark.sh

# Comprehensive benchmarking with detailed analysis
./scripts/benchmark.sh
```

## Benchmarking Infrastructure

### Available Tools

#### 1. `simple_benchmark.sh` - Quick Performance Check
- **Purpose**: Fast overhead measurement for development
- **Runtime**: ~30 seconds
- **Output**: Basic timing and memory usage
- **Use case**: Regular development checks, CI/CD integration

#### 2. `benchmark.sh` - Comprehensive Analysis  
- **Purpose**: Detailed performance analysis with statistical rigor
- **Runtime**: ~5 minutes
- **Output**: CSV data, statistical analysis, performance ratings
- **Use case**: Release validation, performance regression testing

### What Gets Measured

#### Timing Metrics
- **Baseline performance** (hooks disabled)
- **Hook overhead** (hooks enabled, no output)
- **File output overhead** (full instrumentation)
- **Verbose logging overhead** (debug mode)

#### Memory Metrics
- **Peak memory usage** for each test scenario
- **Memory overhead** compared to baseline

#### System Information
- CPU architecture and model
- Available memory
- Operating system details
- Build configuration

## Current Performance Data

### Latest Results (M1 Max, 2025-07-11)

```
Test Configuration:
- System: Darwin arm64 (M1 Max, 32GB RAM)
- Build: Release mode, Metal disabled
- Test: 1024x1024 matrix operations (10 iterations)

Performance Results:
- Baseline (no hooks):  0.246s 
- With file output:     0.256s (+4.1% overhead)
- With verbose:         0.264s (+7.3% overhead)
- Peak memory:          23.5 MB

Event Capture:
- Events recorded:      60 events (WORKING ✅)
- Trace file:           3.3KB (with event data)
- File format:          Valid GGMLVIZ1 with full event stream
- Operations captured:  Matrix multiplications and additions
```

### Performance Rating: ✅ **EXCELLENT**

The current measurements show excellent overhead (<10%) with **fully functional event capture**. The instrumentation is production-ready.

## Current Status: Production Ready ✅

### ✅ Fixed: Event Capture System

**Solution**: All instrumentation hooks are now working correctly.

**Current Status**:
- Hook system initializes and captures events properly
- Trace files contain full event data (60 events for test workload)
- **Full event recording** during computation
- Complete operation timeline capture for visualization

**Impact**: 
- Performance numbers are accurate and representative
- Core functionality is fully operational
- Users can visualize GGML operations in real-time

### ✅ Fixed: Environment Variable Support

**Solution**: `GGML_VIZ_OUTPUT` and all environment variables working correctly.

**Current Status**:
- Environment variables properly parsed and respected
- Users can control output file location
- Full configuration support via environment variables
- Comprehensive help documentation in CLI

## Benchmark Methodology

### Test Environment Requirements

**Prerequisites**:
- Built project with working test executables
- `bc` command for mathematical calculations
- Sufficient disk space for trace files
- No CPU-intensive background processes

**Platform Support**:
- ✅ macOS (Intel & Apple Silicon)
- ✅ Linux (x86_64)
- 🚧 Windows (untested)

### Measurement Approach

#### Statistical Rigor
- **5 iterations** per test (configurable)
- **2 warmup runs** to eliminate cold-start effects
- **Standard deviation calculation** for timing variance
- **Outlier detection** (times >2x baseline flagged)

#### Memory Monitoring
- **Peak memory tracking** during execution
- **Platform-specific tools** (GNU time preferred, ps fallback)
- **Background process isolation**

#### File System Analysis
- **Trace file size measurement**
- **Binary format validation**
- **Compression ratio analysis** (planned)

### Accuracy Limitations

#### Known Sources of Variance
- **Background processes** can cause 2-10x timing spikes
- **Thermal throttling** on sustained workloads
- **Memory allocation patterns** vary between runs
- **System load** affects measurement consistency

#### Mitigation Strategies
- Multiple iterations with statistical analysis
- Warmup runs to stabilize performance
- System information capture for context
- Outlier flagging and removal

## Integration with Development Workflow

### Continuous Integration

```yaml
# Example GitHub Actions integration
- name: Performance Regression Test
  run: |
    ./scripts/simple_benchmark.sh > perf_results.txt
    # Add regression detection logic
```

### Release Validation

```bash
# Pre-release performance validation
./scripts/benchmark.sh
# Review CSV results for acceptable overhead
# Update documentation with new numbers
```

### Development Workflow

```bash
# Before making performance-sensitive changes
./scripts/simple_benchmark.sh > before.txt

# Make changes...

# After changes
./scripts/simple_benchmark.sh > after.txt
diff before.txt after.txt
```

## Interpreting Results

### Performance Thresholds

| Overhead | Rating | Recommendation |
|----------|--------|----------------|
| <5% | Excellent | Production ready |
| 5-10% | Good | Acceptable for most use cases |
| 10-20% | Acceptable | Consider for non-critical paths |
| >20% | High | Optimization needed |

### Red Flags

- **Inconsistent timings** (high standard deviation)
- **Memory leaks** (growing memory usage)
- **Failed trace file creation** (initialization failure)
- **Missing event data** (check hook initialization)

## Future Improvements

### Planned Enhancements

1. **Real-world benchmarks** with actual llama.cpp integration
2. **Automated regression detection** in CI
3. **Cross-platform validation** on Windows/Linux
4. **Model size scaling tests** (1B, 7B, 13B parameters)
5. **GPU backend benchmarking** (Metal, CUDA, Vulkan)

### Benchmark Expansion

```bash
# Planned additional tests
./scripts/benchmark_llama.sh     # Real llama.cpp integration
./scripts/benchmark_memory.sh    # Detailed memory analysis  
./scripts/benchmark_scaling.sh   # Model size scaling
./scripts/benchmark_backends.sh  # GPU backend comparison
```

## Troubleshooting

### Common Issues

#### "No events recorded"
```bash
# Check hook initialization
GGML_VIZ_VERBOSE=1 ./bin/test_ggml_hook | grep -i hook

# Verify file creation and event count
ls -la *.ggmlviz
./bin/test_trace_reader test_trace.ggmlviz

# Should show "Event count: 60" for working system
```

#### "bc command not found"
```bash
# macOS
brew install bc

# Ubuntu/Debian  
sudo apt install bc

# Alternative: use Python for calculations
python3 -c "print(($time2 - $time1) / $time1 * 100)"
```

#### Inconsistent timing results
```bash
# Check system load
top -l 1 | head -10

# Run with higher iteration count
# Edit ITERATIONS=10 in benchmark.sh

# Check for thermal throttling
sudo powermetrics -n 1 -i 1000 | grep -i thermal
```

### Debug Output

Enable verbose logging for detailed debugging:
```bash
GGML_VIZ_VERBOSE=1 ./scripts/simple_benchmark.sh
```

## Contributing

When modifying benchmarking infrastructure:

1. **Test on multiple platforms** (macOS, Linux minimum)
2. **Validate with known-good baselines** 
3. **Document methodology changes**
4. **Update this guide** with new findings
5. **Add regression tests** for benchmark stability

## References

- [Keep a Changelog](https://keepachangelog.com/) - Documentation format
- [Statistical benchmarking best practices](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [Performance testing methodology](https://docs.github.com/en/actions/monitoring-and-troubleshooting-workflows/about-monitoring-and-troubleshooting)

---

**Last Updated**: 2025-07-11  
**Next Review**: After major feature additions or performance regressions