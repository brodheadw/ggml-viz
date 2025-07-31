# GGML Visualizer - Demo Runner (Windows PowerShell)
$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

Write-Host "🚀 GGML Visualizer - Demo Runner (Windows)" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Choose a demo to run:" -ForegroundColor Cyan
Write-Host ""
Write-Host "1) 🦙 LLaMA Demo - Real language model inference" -ForegroundColor Yellow
Write-Host "   • Downloads TinyLlama 1.1B model (~637MB)" -ForegroundColor White
Write-Host "   • Runs actual transformer inference with attention" -ForegroundColor White
Write-Host "   • Generates real GGML computation events" -ForegroundColor White
Write-Host ""
Write-Host "2) 🎤 Whisper Demo - Real audio transcription" -ForegroundColor Yellow
Write-Host "   • Downloads Whisper base.en model (~148MB)" -ForegroundColor White
Write-Host "   • Transcribes sample JFK speech audio" -ForegroundColor White
Write-Host "   • Generates real audio processing events" -ForegroundColor White
Write-Host ""
Write-Host "3) ℹ️  Show system requirements" -ForegroundColor Yellow
Write-Host ""
Write-Host "4) 🚪 Exit" -ForegroundColor Yellow
Write-Host ""

$choice = Read-Host "Enter your choice (1-4)"

switch ($choice) {
    "1" {
        Write-Host ""
        Write-Host "🦙 Starting LLaMA demo..." -ForegroundColor Green
        Write-Host "This will download and build llama.cpp if needed." -ForegroundColor Yellow
        Write-Host "Estimated time: 5-15 minutes (depending on internet speed)" -ForegroundColor Yellow
        Write-Host ""
        $confirm = Read-Host "Continue? (y/N)"
        if ($confirm -match "^[Yy]$") {
            $LlamaScript = Join-Path $ScriptDir "llama_demo\run_llama_demo.ps1"
            if (Test-Path $LlamaScript) {
                & $LlamaScript
            } else {
                Write-Host "❌ Error: LLaMA demo script not found: $LlamaScript" -ForegroundColor Red
            }
        } else {
            Write-Host "Demo cancelled." -ForegroundColor Yellow
        }
    }
    "2" {
        Write-Host ""
        Write-Host "🎤 Starting Whisper demo..." -ForegroundColor Green
        Write-Host "This will download and build whisper.cpp if needed." -ForegroundColor Yellow
        Write-Host "Estimated time: 3-10 minutes (depending on internet speed)" -ForegroundColor Yellow
        Write-Host ""
        $confirm = Read-Host "Continue? (y/N)"
        if ($confirm -match "^[Yy]$") {
            $WhisperScript = Join-Path $ScriptDir "whisper_demo\run_whisper_demo.ps1"
            if (Test-Path $WhisperScript) {
                & $WhisperScript
            } else {
                Write-Host "❌ Error: Whisper demo script not found: $WhisperScript" -ForegroundColor Red
            }
        } else {
            Write-Host "Demo cancelled." -ForegroundColor Yellow
        }
    }
    "3" {
        Write-Host ""
        Write-Host "📋 System Requirements:" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Required:" -ForegroundColor Yellow
        Write-Host "• CMake 3.15+" -ForegroundColor White
        Write-Host "• Visual Studio 2019+ with C++ development workload" -ForegroundColor White
        Write-Host "• Git with submodule support" -ForegroundColor White
        Write-Host "• PowerShell 5.1+ (pre-installed on Windows 10+)" -ForegroundColor White
        Write-Host "• 2-4GB disk space (for models and builds)" -ForegroundColor White
        Write-Host "• 4GB+ RAM (for model loading)" -ForegroundColor White
        Write-Host ""
        Write-Host "Platform Support:" -ForegroundColor Yellow
        Write-Host "• ✅ Windows 10+ (x64) - MinHook DLL injection" -ForegroundColor Green
        Write-Host "• ⚠️  Note: Windows DLL injection is experimental" -ForegroundColor Yellow
        Write-Host "• 💡 Alternative: Use WSL for Linux experience" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Network Requirements:" -ForegroundColor Yellow
        Write-Host "• LLaMA demo: ~637MB download" -ForegroundColor White
        Write-Host "• Whisper demo: ~148MB download + ~1MB audio sample" -ForegroundColor White
        Write-Host ""
        Write-Host "Build Requirements:" -ForegroundColor Yellow
        Write-Host "• llama.cpp: ~5-10 minutes compilation" -ForegroundColor White
        Write-Host "• whisper.cpp: ~2-5 minutes compilation" -ForegroundColor White
        Write-Host ""
        Write-Host "Windows-Specific Notes:" -ForegroundColor Yellow
        Write-Host "• DLL injection may not work in all environments" -ForegroundColor White
        Write-Host "• If demos fail, try manual setup (see BUILD.md)" -ForegroundColor White
        Write-Host "• Consider using WSL for full Linux compatibility" -ForegroundColor White
        Write-Host ""
    }
    "4" {
        Write-Host "Goodbye! 👋" -ForegroundColor Green
        exit 0
    }
    default {
        Write-Host "❌ Invalid choice. Please select 1-4." -ForegroundColor Red
        exit 1
    }
}