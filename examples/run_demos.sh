#!/bin/bash
# GGML Visualizer - Demo Runner
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "🚀 GGML Visualizer - Demo Runner"
echo "================================"
echo
echo "Choose a demo to run:"
echo
echo "1) 🦙 LLaMA Demo - Real language model inference"
echo "   • Downloads TinyLlama 1.1B model (~637MB)"
echo "   • Runs actual transformer inference with attention"
echo "   • Generates real GGML computation events"
echo
echo "2) 🎤 Whisper Demo - Real audio transcription"  
echo "   • Downloads Whisper base.en model (~148MB)"
echo "   • Transcribes sample JFK speech audio"
echo "   • Generates real audio processing events"
echo
echo "3) ℹ️  Show system requirements"
echo
echo "4) 🚪 Exit"
echo

read -p "Enter your choice (1-4): " choice

case $choice in
    1)
        echo
        echo "🦙 Starting LLaMA demo..."
        echo "This will download and build llama.cpp if needed."
        echo "Estimated time: 5-15 minutes (depending on internet speed)"
        echo
        read -p "Continue? (y/N): " confirm
        if [[ $confirm =~ ^[Yy]$ ]]; then
            exec "${SCRIPT_DIR}/llama_demo/run_llama_demo.sh"
        else
            echo "Demo cancelled."
        fi
        ;;
    2)
        echo
        echo "🎤 Starting Whisper demo..."
        echo "This will download and build whisper.cpp if needed."
        echo "Estimated time: 3-10 minutes (depending on internet speed)"
        echo
        read -p "Continue? (y/N): " confirm
        if [[ $confirm =~ ^[Yy]$ ]]; then
            exec "${SCRIPT_DIR}/whisper_demo/run_whisper_demo.sh"
        else
            echo "Demo cancelled."
        fi
        ;;
    3)
        echo
        echo "📋 System Requirements:"
        echo
        echo "Required:"
        echo "• CMake 3.15+"
        echo "• C++17 compiler (GCC 8+, Clang 10+, MSVC 2019+)"
        echo "• Git with submodule support"
        echo "• wget or curl (for downloads)"
        echo "• 2-4GB disk space (for models and builds)"
        echo "• 4GB+ RAM (for model loading)"
        echo
        echo "Platform Support:"
        echo "• ✅ macOS (arm64/x64) - DYLD_INSERT_LIBRARIES"
        echo "• ✅ Linux (x64) - LD_PRELOAD"  
        echo "• ⚠️  Windows - MinHook (experimental)"
        echo
        echo "Network Requirements:"
        echo "• LLaMA demo: ~637MB download"
        echo "• Whisper demo: ~148MB download + ~1MB audio sample"
        echo
        echo "Build Requirements:"
        echo "• llama.cpp: ~5-10 minutes compilation"
        echo "• whisper.cpp: ~2-5 minutes compilation"
        echo
        ;;
    4)
        echo "Goodbye! 👋"
        exit 0
        ;;
    *)
        echo "❌ Invalid choice. Please select 1-4."
        exit 1
        ;;
esac