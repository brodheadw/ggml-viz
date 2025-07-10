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

We provide automated tools to help maintain consistent code style:

```bash
# Auto-format code (requires clang-format)
./scripts/format.sh

# Run static analysis and linting
./scripts/lint.sh

# Run test suite
./scripts/run_tests.sh
```

- Follow existing code style and formatting
- Add appropriate comments for complex logic
- Update documentation for user-facing changes

## Current Development Priorities

See [TODO.md](../TODO.md) for the complete list of current development priorities and tasks. This includes:

- High-priority features and infrastructure improvements
- Medium-priority advanced features
- Low-priority nice-to-have enhancements

The TODO.md file is actively maintained and provides the most up-to-date view of what needs to be done.

## Reporting Issues

- Use GitHub Issues (when repository is published)
- Include build environment details
- Provide steps to reproduce
- Include relevant logs with `GGML_VIZ_VERBOSE=1`

## License

By contributing, you agree that your contributions will be licensed under the Apache 2.0 License.

---

**Note**: This project has reached production readiness with live mode fully functional on macOS. Core development tools (formatting, linting, testing) are implemented. See [TODO.md](../TODO.md) for remaining features and priorities.