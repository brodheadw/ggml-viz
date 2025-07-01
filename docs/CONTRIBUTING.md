# Contributing to GGML Visualizer

Thank you for your interest in contributing to GGML Visualizer!

## Quick Start

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Make your changes
4. Build and test: `cd build && make -j4 && ./bin/test_ggml_hook`
5. Commit your changes: `git commit -m "Add your feature"`
6. Push to your fork: `git push origin feature/your-feature`
7. Create a pull request

## Development Setup

See the main README.md for build instructions.

### macOS
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DGGML_METAL=OFF
make -j4
```

### Linux
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4
```

## Testing

Run the test suite:
```bash
./bin/test_ggml_hook
./bin/test_trace_reader test_trace.ggmlviz
```

## Code Style

- Follow existing code style and formatting
- Add appropriate comments for complex logic
- Update documentation for user-facing changes

## Areas We Need Help

1. **Performance benchmarking** - Measure actual overhead
2. **Development scripts** - Implement linting, formatting, testing scripts
3. **Example integrations** - Real llama.cpp/whisper.cpp demos
4. **Advanced visualizations** - Timeline, tensor stats, memory tracking
5. **Cross-platform testing** - Windows, Linux validation

## Reporting Issues

- Use GitHub Issues (when repository is published)
- Include build environment details
- Provide steps to reproduce
- Include relevant logs with `GGML_VIZ_VERBOSE=1`

## License

By contributing, you agree that your contributions will be licensed under the Apache 2.0 License.

---

**Note**: This project is in active development. Many development tools (linting, CI, etc.) are still being implemented. See TODO.md for current priorities.