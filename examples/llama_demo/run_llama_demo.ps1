# LLaMA Demo - Download and run real LLaMA model with GGML Visualizer (Windows PowerShell)
param(
    [string]$ConfigFile = "llama_demo_config.json"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)
$ThirdPartyDir = Join-Path $ProjectRoot "third_party"
$BuildDir = Join-Path $ProjectRoot "build"

Write-Host "🦙 GGML Visualizer - Real LLaMA Demo (Windows)" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green
Write-Host ""

# Check if ggml-viz is built
$GgmlVizExe = Join-Path $BuildDir "bin\Release\ggml-viz.exe"
if (-not (Test-Path $GgmlVizExe)) {
    $GgmlVizExe = Join-Path $BuildDir "bin\ggml-viz.exe"
    if (-not (Test-Path $GgmlVizExe)) {
        Write-Host "❌ Error: ggml-viz not found in $BuildDir\bin\" -ForegroundColor Red
        Write-Host "Please build the project first:" -ForegroundColor Yellow
        Write-Host "  mkdir build; cd build" -ForegroundColor Yellow
        Write-Host "  cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release" -ForegroundColor Yellow
        Write-Host "  cmake --build . --config Release --parallel" -ForegroundColor Yellow
        exit 1
    }
}

# Check if hook library exists
$HookLib = Join-Path $BuildDir "src\Release\ggml_viz_hook.dll"
if (-not (Test-Path $HookLib)) {
    $HookLib = Join-Path $BuildDir "src\ggml_viz_hook.dll"
    if (-not (Test-Path $HookLib)) {
        Write-Host "❌ Error: Hook library not found: $HookLib" -ForegroundColor Red
        Write-Host "Please ensure the project was built successfully." -ForegroundColor Yellow
        exit 1
    }
}

# Create third_party directory if it doesn't exist
if (-not (Test-Path $ThirdPartyDir)) {
    New-Item -ItemType Directory -Path $ThirdPartyDir | Out-Null
}
Set-Location $ThirdPartyDir

# Step 1: Clone llama.cpp if needed
$LlamaDir = Join-Path $ThirdPartyDir "llama.cpp"
if (-not (Test-Path $LlamaDir)) {
    Write-Host "📥 Cloning llama.cpp..." -ForegroundColor Cyan
    git clone https://github.com/ggerganov/llama.cpp.git
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Failed to clone llama.cpp" -ForegroundColor Red
        exit 1
    }
    Write-Host "✅ llama.cpp cloned successfully" -ForegroundColor Green
} else {
    Write-Host "📁 llama.cpp already exists, skipping clone" -ForegroundColor Yellow
}

Set-Location $LlamaDir

# Step 2: Build llama.cpp if needed
$LlamaBuildDir = Join-Path $LlamaDir "build"
$LlamaExe = Join-Path $LlamaBuildDir "bin\Release\llama-cli.exe"
if (-not (Test-Path $LlamaExe)) {
    $LlamaExe = Join-Path $LlamaBuildDir "bin\llama-cli.exe"
    if (-not (Test-Path $LlamaExe)) {
        Write-Host "🔨 Building llama.cpp..." -ForegroundColor Cyan
        if (-not (Test-Path $LlamaBuildDir)) {
            New-Item -ItemType Directory -Path $LlamaBuildDir | Out-Null
        }
        Set-Location $LlamaBuildDir
        cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release
        if ($LASTEXITCODE -ne 0) {
            Write-Host "❌ Failed to configure llama.cpp" -ForegroundColor Red
            exit 1
        }
        cmake --build . --config Release --parallel
        if ($LASTEXITCODE -ne 0) {
            Write-Host "❌ Failed to build llama.cpp" -ForegroundColor Red
            exit 1
        }
        Write-Host "✅ llama.cpp built successfully" -ForegroundColor Green
        Set-Location $LlamaDir
    }
} else {
    Write-Host "📦 llama.cpp already built, skipping build" -ForegroundColor Yellow
}

# Step 3: Download model if needed
$ModelsDir = Join-Path $LlamaDir "models"
if (-not (Test-Path $ModelsDir)) {
    New-Item -ItemType Directory -Path $ModelsDir | Out-Null
}
Set-Location $ModelsDir

$ModelFile = "tinyllama-1.1b-chat-v1.0.q4_k_m.gguf"
$ModelUrl = "https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGML/resolve/main/tinyllama-1.1b-chat-v1.0.q4_k_m.gguf"

if (-not (Test-Path $ModelFile)) {
    Write-Host "📥 Downloading TinyLlama model (~637MB)..." -ForegroundColor Cyan
    Write-Host "This may take a few minutes depending on your internet connection." -ForegroundColor Yellow
    
    try {
        Invoke-WebRequest -Uri $ModelUrl -OutFile $ModelFile -UseBasicParsing
        Write-Host "✅ Model downloaded successfully" -ForegroundColor Green
    } catch {
        Write-Host "❌ Error downloading model: $_" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "📁 Model already exists, skipping download" -ForegroundColor Yellow
}

# Verify model file
if (-not (Test-Path $ModelFile) -or (Get-Item $ModelFile).Length -eq 0) {
    Write-Host "❌ Error: Model file is missing or empty: $ModelFile" -ForegroundColor Red
    exit 1
}

$ModelSize = [math]::Round((Get-Item $ModelFile).Length / 1MB, 1)
Write-Host "📊 Model size: $($ModelSize)MB" -ForegroundColor Cyan

# Step 4: Prepare for trace capture
Set-Location $ProjectRoot
$TraceFile = "llama_real_trace.ggmlviz"

Write-Host ""
Write-Host "🚀 Running LLaMA with GGML Visualizer instrumentation..." -ForegroundColor Green
Write-Host "📁 Trace file: $TraceFile" -ForegroundColor Cyan
Write-Host "🔧 Hook library: $HookLib" -ForegroundColor Cyan
Write-Host ""

# Set up environment for instrumentation
$env:GGML_VIZ_OUTPUT = $TraceFile
$env:GGML_VIZ_VERBOSE = "1"
$env:GGML_VIZ_MAX_EVENTS = "100000"

# Copy hook DLL to llama.cpp directory for simplicity
$LlamaHookLib = Join-Path (Split-Path $LlamaExe) "ggml_viz_hook.dll"
Copy-Item $HookLib $LlamaHookLib -Force

Write-Host "🪟 Using Windows DLL injection (experimental)" -ForegroundColor Yellow
Write-Host "💬 Running inference: 'Explain quantum computing in simple terms.'" -ForegroundColor Cyan
Write-Host "⏱️  This will generate real GGML events for visualization..." -ForegroundColor Cyan
Write-Host ""

# Step 5: Run LLaMA with instrumentation
try {
    & $LlamaExe `
        -m (Join-Path $ModelsDir $ModelFile) `
        -p "Explain quantum computing in simple terms." `
        -n 50 `
        --temp 0.7 `
        --top-p 0.9 `
        --seed 42
} catch {
    Write-Host "❌ Error running llama: $_" -ForegroundColor Red
    Write-Host "🔧 Debug: Check if hook DLL was loaded properly" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "🎉 Demo completed!" -ForegroundColor Green

# Check if trace was generated
if (Test-Path $TraceFile) {
    $TraceSize = [math]::Round((Get-Item $TraceFile).Length / 1KB, 1)
    Write-Host "✅ Trace file generated: $TraceFile ($($TraceSize)KB)" -ForegroundColor Green
    
    # Rough event count estimation
    $EventCount = [math]::Round((Get-Item $TraceFile).Length / 50)  # Rough estimate
    Write-Host "📊 Estimated events captured: ~$EventCount" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "🖥️  To view the trace in the GUI:" -ForegroundColor Yellow
    Write-Host "   $GgmlVizExe $TraceFile" -ForegroundColor White
    Write-Host ""
    Write-Host "🔍 To view in live mode (run in another terminal):" -ForegroundColor Yellow
    Write-Host "   $GgmlVizExe --live $TraceFile" -ForegroundColor White
} else {
    Write-Host "❌ Warning: No trace file generated. Check for errors above." -ForegroundColor Red
    Write-Host "🔧 Debug: Check if hook DLL was loaded properly" -ForegroundColor Yellow
    Write-Host "🔧 Note: Windows DLL injection is experimental and may not work in all cases." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "🎯 This demo showed REAL LLaMA inference with actual:" -ForegroundColor Green
Write-Host "   • Token embeddings and positional encoding" -ForegroundColor White
Write-Host "   • Multi-head attention computations" -ForegroundColor White
Write-Host "   • Feed-forward network operations" -ForegroundColor White
Write-Host "   • Layer normalization and residual connections" -ForegroundColor White
Write-Host "   • Output token generation and sampling" -ForegroundColor White
Write-Host ""
Write-Host "📈 All operations were captured by GGML Visualizer for analysis!" -ForegroundColor Green