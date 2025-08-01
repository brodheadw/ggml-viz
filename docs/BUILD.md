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

Windows users have several build options depending on their setup and preferences:

#### Option A: Visual Studio Build Tools (Recommended)
**Lightweight Microsoft compiler without the full IDE (~2GB)**
- **Visual Studio Build Tools 2022** with C++ workload
- **CMake 3.15 or later** (included with Build Tools)
- **Git** with submodule support

```powershell
# Install via WinGet (requires Windows Package Manager)
winget install --id Microsoft.VisualStudio.2022.BuildTools -e
# In the installer: check "Desktop development with C++" → Install
```

#### Option B: Full Visual Studio IDE
**If you already have or want the full development environment**
- **Visual Studio 2019 or later** with C++ development workload
- **CMake 3.15 or later**
- **Git** with submodule support

#### Option C: MinGW-w64 via MSYS2
**Open-source GCC toolchain for Windows (no Microsoft dependencies)**
- **MSYS2** environment with MinGW-w64 toolchain
- **CMake** and **Ninja** build system
- Works without admin privileges or Microsoft toolchain

```bash
# Install MSYS2 from https://www.msys2.org
# In MSYS2 MinGW 64-bit terminal:
pacman -Sy --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
                     mingw-w64-x86_64-ninja mingw-w64-x86_64-openmp
```

#### Option D: WSL2 + Linux
**Build Linux binaries inside Windows Subsystem for Linux**
- **WSL2** with Ubuntu or similar distribution
- Follow Linux build instructions inside WSL
- Requires X-server (VcXsrv, X410) for GUI display

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

### Windows Build

Choose one of the following approaches based on your installed toolchain:

#### Option A: Visual Studio Build Tools or Full IDE (Git Bash)
```bash
# Open "x64 Native Tools Command Prompt for VS 2022" then run: bash
mkdir build && cd build

# Configure for x64 with Visual Studio generator
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
# For VS 2019: cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release

# Build with parallel compilation
cmake --build . --config Release --parallel
```

#### Option A Alternative: Using Ninja (Fastest)
```bash
# Open "x64 Native Tools Command Prompt for VS 2022" then run: bash
mkdir build && cd build

# Configure with Ninja generator (faster than Visual Studio generator)
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

#### Option B: PowerShell/CMD with Visual Studio
```powershell
mkdir build
cd build

# Configure for x64 architecture  
cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release

# Build with parallel compilation
cmake --build . --config Release --parallel
```

#### Option C: MinGW-w64 via MSYS2
```bash
# In MSYS2 MinGW 64-bit terminal
mkdir build && cd build

# Configure with MinGW-w64 GCC
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build . --parallel
```

#### Option D: WSL2 + Linux
```bash
# Follow the Linux build instructions inside WSL2
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
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

## Building llama.cpp for Testing

To test ggml-viz with real workloads, you'll need to build llama.cpp:

### Download and Build llama.cpp

#### Unix/Linux/macOS
```bash
# Clone llama.cpp in the third_party directory (from ggml-viz root)
cd third_party
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp

# Build llama.cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)  # Linux
# make -j$(sysctl -n hw.ncpu)  # macOS
```

#### Windows (Git Bash with Visual Studio)
```bash
# Open "x64 Native Tools Command Prompt for VS 2022" then run: bash
mkdir -p third_party && cd third_party
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp

# Configure and build with same method used for ggml-viz
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel

# Return to ggml-viz root
cd ../../
```

#### Windows (MinGW-w64 via MSYS2)
```bash
# In MSYS2 MinGW 64-bit terminal
mkdir -p third_party && cd third_party
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp

# Build with MinGW-w64
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build --parallel

cd ../../
```

### Download a Test Model

#### Method 1: Hugging Face CLI (Recommended - all platforms)
```bash
# Install the CLI (one-time setup)
python -m pip install -U "huggingface_hub[cli]"

# Authenticate (one-time setup) - creates browser login
hf login

# Create models directory
mkdir -p third_party/llama.cpp/models

# Download TinyLlama 1.1B quantized model (~670MB)
hf download TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF \
            tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
            --local-dir third_party/llama.cpp/models
```

#### Method 2: Direct Download (Unix/Linux/macOS)
```bash
# Create models directory and download with wget/curl
mkdir -p third_party/llama.cpp/models
cd third_party/llama.cpp/models

# Using wget
wget https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf

# Or using curl
curl -L -o tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
     https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf

cd ../../../  # Return to ggml-viz root
```

#### Method 3: PowerShell (Windows)
```powershell
# With authentication token (get from https://huggingface.co/settings/tokens)
$env:HF_TOKEN = "hf_your_token_here"
New-Item -ItemType Directory -Force -Path "third_party\llama.cpp\models"

Invoke-WebRequest -Uri "https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf" `
                  -Headers @{ Authorization = "Bearer $env:HF_TOKEN" } `
                  -OutFile "third_party\llama.cpp\models\tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
```

### Test with ggml-viz

#### Unix/Linux/macOS
```bash
# From ggml-viz root directory
# Terminal 1: Run llama.cpp with ggml-viz instrumentation
env GGML_VIZ_OUTPUT=test_live_trace.ggmlviz \
    DYLD_INSERT_LIBRARIES=./build/src/libggml_viz_hook.dylib \  # macOS
    # LD_PRELOAD=./build/src/libggml_viz_hook.so \              # Linux
    ./third_party/llama.cpp/build/bin/llama-cli \
    -m ./third_party/llama.cpp/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
    -p "Hello world" -n 10 --verbose-prompt

# Terminal 2: View live trace
./build/bin/ggml-viz --live test_live_trace.ggmlviz --no-hook
```

#### Windows (Git Bash)
```bash
# From ggml-viz root directory
# Add hook DLL to PATH so Windows can find it
export PATH="$PWD/build/bin/Release:$PATH"

# Set trace output file
export GGML_VIZ_OUTPUT="$PWD/test_live_trace.ggmlviz"

# Terminal 1: Run llama.cpp with ggml-viz instrumentation
third_party/llama.cpp/build/bin/Release/main.exe \
  -m third_party/llama.cpp/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
  -p "Hello world" -n 10 --verbose-prompt

# Terminal 2: View live trace  
build/bin/Release/ggml-viz.exe --live "$PWD/test_live_trace.ggmlviz" --no-hook
```

#### Windows (PowerShell)
```powershell
# From ggml-viz root directory
# Add hook DLL to PATH
$env:PATH = "$PWD\build\bin\Release;$env:PATH"

# Set trace output file
$env:GGML_VIZ_OUTPUT = "$PWD\test_live_trace.ggmlviz"

# Terminal 1: Run llama.cpp with ggml-viz instrumentation
.\third_party\llama.cpp\build\bin\Release\main.exe `
  -m .\third_party\llama.cpp\models\tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf `
  -p "Hello world" -n 10 --verbose-prompt

# Terminal 2: View live trace
.\build\bin\Release\ggml-viz.exe --live "$PWD\test_live_trace.ggmlviz" --no-hook
```

#### Windows (MinGW-w64 via MSYS2)
```bash
# From ggml-viz root directory in MSYS2 MinGW 64-bit terminal
export PATH="$PWD/build:$PATH"
export GGML_VIZ_OUTPUT="$PWD/test_live_trace.ggmlviz"

# Terminal 1: Run with MinGW-w64 built binaries
third_party/llama.cpp/build/main.exe \
  -m third_party/llama.cpp/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
  -p "Hello world" -n 10 --verbose-prompt

# Terminal 2: View live trace
build/ggml-viz.exe --live "$PWD/test_live_trace.ggmlviz" --no-hook
```

## Testing the Build

### Real Model Demo Applications (Recommended)
The project includes demo scripts that download and run real GGML models for authentic testing:

#### **Unix/Linux/macOS**
```bash
# Interactive demo menu with system requirements
./examples/run_demos.sh

# Real LLaMA demo - Downloads TinyLlama 1.1B (~637MB) and runs inference
./examples/llama_demo/run_llama_demo.sh
# Generates llama_real_trace.ggmlviz with actual transformer operations

# Real Whisper demo - Downloads Whisper base.en (~148MB) and transcribes audio
./examples/whisper_demo/run_whisper_demo.sh  
# Generates whisper_real_trace.ggmlviz with actual audio processing

# View the real traces in the GUI
./bin/ggml-viz llama_real_trace.ggmlviz
./bin/ggml-viz whisper_real_trace.ggmlviz
```

#### **Windows (PowerShell)**
```powershell
# Interactive demo menu with system requirements
.\examples\run_demos.ps1

# Real LLaMA demo - Downloads TinyLlama 1.1B (~637MB) and runs inference
.\examples\llama_demo\run_llama_demo.ps1
# Generates llama_real_trace.ggmlviz with actual transformer operations

# Real Whisper demo - Downloads Whisper base.en (~148MB) and transcribes audio
.\examples\whisper_demo\run_whisper_demo.ps1
# Generates whisper_real_trace.ggmlviz with actual audio processing

# View the real traces in the GUI
.\bin\Release\ggml-viz.exe llama_real_trace.ggmlviz
.\bin\Release\ggml-viz.exe whisper_real_trace.ggmlviz
```

#### **Windows (WSL Alternative)**
If PowerShell scripts don't work, use Windows Subsystem for Linux:
```bash
# Run in WSL
./examples/run_demos.sh  # Uses Linux approach
```

**System Requirements for Real Demos**:
- 2-4GB disk space (models + builds)
- 4GB+ RAM for model loading
- Internet connection for downloads
- Unix: wget or curl for downloading
- Windows: PowerShell 5.1+ (pre-installed on Windows 10+)

### Configuration File Examples
The demos use JSON configuration files that demonstrate the full configuration schema:

**LLaMA Demo Configuration** (`examples/llama_demo/llama_demo_config.json`):
```json
{
  "cli": {
    "verbose": true,
    "live_mode": false,
    "port": 8080
  },
  "instrumentation": {
    "max_events": 50000,
    "enable_op_timing": true,
    "enable_memory_tracking": true,
    "output_file": "llama_trace.ggmlviz"
  },
  "logging": {
    "level": "INFO",
    "enable_timestamps": true,
    "enable_thread_id": false,
    "prefix": "[LLAMA_DEMO]"
  }
}
```

**Whisper Demo Configuration** (`examples/whisper_demo/whisper_demo_config.json`):
```json
{
  "cli": {
    "verbose": true,
    "live_mode": true,
    "port": 8081,
    "polling_interval_ms": 75
  },
  "instrumentation": {
    "max_events": 40000,
    "enable_op_timing": true,
    "enable_memory_tracking": true,
    "output_file": "whisper_trace.ggmlviz"
  },
  "logging": {
    "level": "DEBUG",
    "enable_timestamps": true,
    "enable_thread_id": true,
    "prefix": "[WHISPER_DEMO]"
  }
}
```

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

### Windows-Specific Demo Notes

#### **DLL Injection Limitations**
Windows demos use experimental DLL injection that may not work in all environments:
- **MinHook**: Experimental runtime API patching
- **Security Software**: Antivirus may block DLL injection
- **Permissions**: May require administrator privileges

#### **Troubleshooting Windows Demos**
If PowerShell demos fail:

1. **Check Hook DLL**: Ensure `ggml_viz_hook.dll` exists in build output
   ```powershell
   Test-Path "build\src\Release\ggml_viz_hook.dll"
   ```

2. **Manual Setup**: Run llama.cpp/whisper.cpp manually with DLL in same directory
   ```powershell
   # Copy hook DLL to same directory as executable
   Copy-Item "build\src\Release\ggml_viz_hook.dll" "third_party\llama.cpp\build\bin\Release\"
   
   # Set environment and run
   $env:GGML_VIZ_OUTPUT = "trace.ggmlviz"
   $env:GGML_VIZ_VERBOSE = "1"
   .\third_party\llama.cpp\build\bin\Release\llama-cli.exe -m model.gguf -p "Hello"
   ```

3. **Use WSL**: For full Linux compatibility
   ```bash
   # In WSL
   ./examples/run_demos.sh
   ```

4. **Manual Integration**: Build and run models separately, then use function interposition from documentation

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

**Windows build errors:**

*CMake generator conflicts:*
```bash
# Error: "Configuring incomplete, errors occurred!" or generator mismatch
# Solution: Clean build directory when changing generators
rm -rf build  # or: rmdir /s build in CMD
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
```

*Visual Studio not found:*
```bash
# Error: "Visual Studio 17 2022 could not be found"
# Solution A: Install Build Tools (recommended)
winget install --id Microsoft.VisualStudio.2022.BuildTools -e

# Solution B: Use older version you have installed
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release

# Solution C: Switch to MinGW-w64
# Install MSYS2, then in MinGW 64-bit terminal:
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
```

*Missing compiler or SDK:*
```bash
# Error: "The C compiler [...] is not able to compile a simple test program"
# Solution: Use proper development command prompt
# Start Menu → "x64 Native Tools Command Prompt for VS 2022" → bash
```

*DLL/executable bitness mismatch:*
```bash
# Error: Hook DLL fails to load or "incorrect format" 
# Solution: Ensure both ggml-viz and target application are same architecture
cmake .. -A x64  # Force 64-bit build
# Check target app: dumpbin /headers your_app.exe | findstr machine
```

*MinGW-w64 specific issues:*
```bash
# Missing runtime DLLs when distributing
# Copy these DLLs alongside your .exe:
# - libgcc_s_seh-1.dll
# - libstdc++-6.dll  
# - libwinpthread-1.dll
# - libgomp-1.dll (if using OpenMP)
```

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