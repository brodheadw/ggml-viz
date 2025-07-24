# Build Guide

This guide provides detailed instructions for building ggml-viz from source on all supported platforms.

## Prerequisites

### macOS
```bash
# Install dependencies via Homebrew
brew install cmake glfw

# Xcode Command Line Tools (if not already installed)
xcode-select --install
```

### Linux (Ubuntu/Debian)
```bash
# Update package lists and install build dependencies
sudo apt update && sudo apt install -y \
    git \
    cmake \
    build-essential \
    libgl1-mesa-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxrandr-dev
```

### Linux (CentOS/RHEL/Fedora)
```bash
# CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install cmake mesa-libGL-devel libXinerama-devel libXcursor-devel libXi-devel libXrandr-devel

# Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake mesa-libGL-devel libXinerama-devel libXcursor-devel libXi-devel libXrandr-devel
```

### Windows
- **Visual Studio 2019 or later** with C++ development workload
- **CMake 3.15 or later**
- **Git** with submodule support

## Building from Source

### Clone Repository
```bash
git clone --recursive https://github.com/brodheadw/ggml-viz.git
cd ggml-viz
```

**Important**: Use `--recursive` to clone the GGML submodule. If you forgot, run:
```bash
git submodule update --init --recursive
```

### macOS Build
```bash
mkdir build && cd build

# Configure build (Metal disabled due to shader compilation issues)
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF

# Build with parallel jobs
make -j$(sysctl -n hw.ncpu)
```

### Linux Build
```bash
mkdir build && cd build

# Configure build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build with all available cores
make -j$(nproc)
```

### Windows Build (PowerShell/CMD)
```powershell
mkdir build
cd build

# Configure for x64 architecture
cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release

# Build with parallel compilation
cmake --build . --config Release --parallel
```

## Build Options

### CMake Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug` | Build type: `Debug`, `Release`, `RelWithDebInfo` |
| `GGML_METAL` | `ON` | Enable Metal backend (macOS only, disable if shader issues) |
| `GGML_CUDA` | `OFF` | Enable CUDA backend (requires CUDA toolkit) |
| `GGML_VULKAN` | `OFF` | Enable Vulkan backend (requires Vulkan SDK) |
| `GGML_VIZ_BUILD_TESTS` | `ON` | Build unit tests and examples |
| `GGML_VIZ_BUILD_EXAMPLES` | `ON` | Build example applications |

### Example Configurations

**Development build with debug symbols:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DGGML_VIZ_BUILD_TESTS=ON
```

**Production build with CUDA support:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_CUDA=ON
```

**Minimal build (no tests/examples):**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_VIZ_BUILD_TESTS=OFF -DGGML_VIZ_BUILD_EXAMPLES=OFF
```

## Testing the Build

### Basic Functionality Tests
```bash
# Unix/Linux/macOS
./bin/ggml-viz --version
./bin/ggml-viz --help

# Windows
.\bin\Release\ggml-viz.exe --version
.\bin\Release\ggml-viz.exe --help
```

### Run Unit Tests
```bash
# Unix/Linux/macOS
cd build
ctest

# Windows
cd build
ctest -C Release
```

### Manual Tests
```bash
# Unix/Linux/macOS
./tests/manual/test_ggml_hook
./bin/ggml-viz tests/assets/test_trace.ggmlviz

# Windows
.\tests\manual\Release\test_ggml_hook.exe
.\bin\Release\ggml-viz.exe tests\assets\test_trace.ggmlviz
```

## GPU Backend Setup

### CUDA Backend (Linux/Windows)
1. Install [CUDA Toolkit 11.8 or later](https://developer.nvidia.com/cuda-downloads)
2. Configure with CUDA enabled:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_CUDA=ON
   ```
3. Ensure CUDA libraries are in your PATH/LD_LIBRARY_PATH

### Vulkan Backend (Cross-platform)
1. Install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
2. Configure with Vulkan enabled:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_VULKAN=ON
   ```

### Metal Backend (macOS)
Metal is enabled by default on macOS but may have shader compilation issues:
```bash
# If you encounter Metal shader errors, disable Metal:
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_METAL=OFF
```

## Troubleshooting

### Common Build Issues

**"Could not find GGML" error:**
```bash
# Ensure submodules are initialized
git submodule update --init --recursive
```

**OpenGL/GUI library errors on Linux:**
```bash
# Install missing GUI dependencies
sudo apt install libgl1-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev
```

**Windows MSVC errors:**
- Ensure Visual Studio 2019 or later is installed
- Use Visual Studio Developer Command Prompt
- Try building with specific generator: `cmake .. -G "Visual Studio 16 2019" -A x64`

### Performance Optimization

**Release builds for performance:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DNDEBUG=1
```

**Link-time optimization (LTO):**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

**Native CPU optimization:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-march=native"
```

## Development Builds

### Debug Build with Sanitizers
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined" \
         -DCMAKE_LINKER_FLAGS="-fsanitize=address -fsanitize=undefined"
```

### Code Coverage Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="--coverage" \
         -DCMAKE_LINKER_FLAGS="--coverage"
```

## Installation

### System-wide Installation
```bash
# After building
sudo make install

# Or with CMake
sudo cmake --install . --config Release
```

### Custom Installation Directory
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/ggml-viz
make install
```

### Package Creation
```bash
# Create distributable package
cpack

# Create specific package type
cpack -G TGZ  # Tarball
cpack -G DEB  # Debian package (Linux)
cpack -G ZIP  # ZIP archive (Windows)
```

## Docker Build

### Using Docker
```bash
# Build Docker image
docker build -t ggml-viz .

# Run container
docker run -it ggml-viz ggml-viz --help
```

See [Docker documentation](DOCKER.md) for detailed container usage.