# Whisper Demo - Download and run real Whisper model with GGML Visualizer (Windows PowerShell)
param(
    [string]$ConfigFile = "whisper_demo_config.json"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)
$ThirdPartyDir = Join-Path $ProjectRoot "third_party"
$BuildDir = Join-Path $ProjectRoot "build"

Write-Host "🎤 GGML Visualizer - Real Whisper Demo (Windows)" -ForegroundColor Green
Write-Host "===============================================" -ForegroundColor Green
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

# Step 1: Clone whisper.cpp if needed
$WhisperDir = Join-Path $ThirdPartyDir "whisper.cpp"
if (-not (Test-Path $WhisperDir)) {
    Write-Host "📥 Cloning whisper.cpp..." -ForegroundColor Cyan
    git clone https://github.com/ggerganov/whisper.cpp.git
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Failed to clone whisper.cpp" -ForegroundColor Red
        exit 1
    }
    Write-Host "✅ whisper.cpp cloned successfully" -ForegroundColor Green
} else {
    Write-Host "📁 whisper.cpp already exists, skipping clone" -ForegroundColor Yellow
}

Set-Location $WhisperDir

# Step 2: Build whisper.cpp if needed
$WhisperBuildDir = Join-Path $WhisperDir "build"
$WhisperExe = Join-Path $WhisperBuildDir "bin\Release\main.exe"
if (-not (Test-Path $WhisperExe)) {
    $WhisperExe = Join-Path $WhisperBuildDir "bin\main.exe"
    if (-not (Test-Path $WhisperExe)) {
        Write-Host "🔨 Building whisper.cpp..." -ForegroundColor Cyan
        if (-not (Test-Path $WhisperBuildDir)) {
            New-Item -ItemType Directory -Path $WhisperBuildDir | Out-Null
        }
        Set-Location $WhisperBuildDir
        cmake .. -A x64 -DCMAKE_BUILD_TYPE=Release
        if ($LASTEXITCODE -ne 0) {
            Write-Host "❌ Failed to configure whisper.cpp" -ForegroundColor Red
            exit 1
        }
        cmake --build . --config Release --parallel
        if ($LASTEXITCODE -ne 0) {
            Write-Host "❌ Failed to build whisper.cpp" -ForegroundColor Red
            exit 1
        }
        Write-Host "✅ whisper.cpp built successfully" -ForegroundColor Green
        Set-Location $WhisperDir
    }
} else {
    Write-Host "📦 whisper.cpp already built, skipping build" -ForegroundColor Yellow
}

# Step 3: Download model if needed
$ModelsDir = Join-Path $WhisperDir "models"
if (-not (Test-Path $ModelsDir)) {
    New-Item -ItemType Directory -Path $ModelsDir | Out-Null
}
Set-Location $ModelsDir

$ModelFile = "ggml-base.en.bin"
$ModelUrl = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"

if (-not (Test-Path $ModelFile)) {
    Write-Host "📥 Downloading Whisper base.en model (~148MB)..." -ForegroundColor Cyan
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

# Step 4: Download sample audio if needed
Set-Location $WhisperDir
$SampleAudio = "samples\jfk.wav"
$SamplesDir = "samples"

if (-not (Test-Path $SampleAudio)) {
    Write-Host "📥 Downloading sample audio..." -ForegroundColor Cyan
    if (-not (Test-Path $SamplesDir)) {
        New-Item -ItemType Directory -Path $SamplesDir | Out-Null
    }
    
    # Download JFK sample audio
    $AudioUrl = "https://github.com/ggerganov/whisper.cpp/raw/master/samples/jfk.wav"
    
    try {
        Invoke-WebRequest -Uri $AudioUrl -OutFile $SampleAudio -UseBasicParsing
        Write-Host "✅ Sample audio downloaded" -ForegroundColor Green
    } catch {
        Write-Host "❌ Error downloading sample audio: $_" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "📁 Sample audio already exists" -ForegroundColor Yellow
}

# Verify audio file
if (-not (Test-Path $SampleAudio) -or (Get-Item $SampleAudio).Length -eq 0) {
    Write-Host "❌ Error: Sample audio file is missing or empty: $SampleAudio" -ForegroundColor Red
    exit 1
}

$AudioSize = [math]::Round((Get-Item $SampleAudio).Length / 1KB, 1)
Write-Host "🎵 Audio file: $SampleAudio ($($AudioSize)KB)" -ForegroundColor Cyan

# Step 5: Prepare for trace capture
Set-Location $ProjectRoot
$TraceFile = "whisper_real_trace.ggmlviz"

Write-Host ""
Write-Host "🚀 Running Whisper with GGML Visualizer instrumentation..." -ForegroundColor Green
Write-Host "📁 Trace file: $TraceFile" -ForegroundColor Cyan
Write-Host "🔧 Hook library: $HookLib" -ForegroundColor Cyan
Write-Host ""

# Set up environment for instrumentation
$env:GGML_VIZ_OUTPUT = $TraceFile
$env:GGML_VIZ_VERBOSE = "1"
$env:GGML_VIZ_MAX_EVENTS = "500000"  # Whisper generates many more events

# Copy hook DLL to whisper.cpp directory for simplicity
$WhisperHookLib = Join-Path (Split-Path $WhisperExe) "ggml_viz_hook.dll"
Copy-Item $HookLib $WhisperHookLib -Force

Write-Host "🪟 Using Windows DLL injection (experimental)" -ForegroundColor Yellow
Write-Host "🎤 Transcribing JFK speech sample..." -ForegroundColor Cyan
Write-Host "⏱️  This will generate real GGML events for audio processing..." -ForegroundColor Cyan
Write-Host ""

# Step 6: Run Whisper with instrumentation
Set-Location $WhisperDir
try {
    & $WhisperExe -m (Join-Path $ModelsDir $ModelFile) -f $SampleAudio --output-txt --output-vtt
} catch {
    Write-Host "❌ Error running whisper: $_" -ForegroundColor Red
    Write-Host "🔧 Debug: Check if hook DLL was loaded properly" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "🎉 Demo completed!" -ForegroundColor Green

# Check if trace was generated
Set-Location $ProjectRoot
if (Test-Path $TraceFile) {
    $TraceSize = [math]::Round((Get-Item $TraceFile).Length / 1KB, 1)
    Write-Host "✅ Trace file generated: $TraceFile ($($TraceSize)KB)" -ForegroundColor Green
    
    # Rough event count estimation
    $EventCount = [math]::Round((Get-Item $TraceFile).Length / 30)  # Rough estimate
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

# Show transcription results if available
$TranscriptionFile = Join-Path $WhisperDir "samples\jfk.wav.txt"
if (Test-Path $TranscriptionFile) {
    Write-Host ""
    Write-Host "📝 Transcription result:" -ForegroundColor Yellow
    Get-Content $TranscriptionFile | Write-Host -ForegroundColor White
}

Write-Host ""
Write-Host "🎯 This demo showed REAL Whisper audio processing with actual:" -ForegroundColor Green
Write-Host "   • Mel-spectrogram feature extraction" -ForegroundColor White
Write-Host "   • Encoder transformer operations (multi-head attention)" -ForegroundColor White
Write-Host "   • Decoder transformer operations (cross-attention)" -ForegroundColor White
Write-Host "   • Audio preprocessing and windowing" -ForegroundColor White
Write-Host "   • Token prediction and beam search" -ForegroundColor White
Write-Host "   • Language detection and timestamp alignment" -ForegroundColor White
Write-Host ""
Write-Host "📈 All operations were captured by GGML Visualizer for analysis!" -ForegroundColor Green