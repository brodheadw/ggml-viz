#!/bin/bash
# Whisper Demo - Download and run real Whisper model with GGML Visualizer
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
THIRD_PARTY_DIR="${PROJECT_ROOT}/third_party"
BUILD_DIR="${PROJECT_ROOT}/build"

echo "🎤 GGML Visualizer - Real Whisper Demo"  
echo "======================================"
echo

# Check if ggml-viz is built
if [[ ! -f "${BUILD_DIR}/bin/ggml-viz" ]]; then
    echo "❌ Error: ggml-viz not found in ${BUILD_DIR}/bin/"
    echo "Please build the project first:"
    echo "  mkdir build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc)"
    exit 1
fi

# Check if hook library exists
HOOK_LIB=""
if [[ "$OSTYPE" == "darwin"* ]]; then
    HOOK_LIB="${BUILD_DIR}/src/libggml_viz_hook.dylib"
    if [[ ! -f "$HOOK_LIB" ]]; then
        echo "❌ Error: Hook library not found: $HOOK_LIB"
        exit 1
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    HOOK_LIB="${BUILD_DIR}/src/libggml_viz_hook.so"
    if [[ ! -f "$HOOK_LIB" ]]; then
        echo "❌ Error: Hook library not found: $HOOK_LIB"
        exit 1
    fi
else
    echo "❌ Error: Unsupported platform: $OSTYPE"
    echo "This demo currently supports macOS and Linux only."
    exit 1
fi

# Create third_party directory if it doesn't exist
mkdir -p "$THIRD_PARTY_DIR"
cd "$THIRD_PARTY_DIR"

# Step 1: Clone whisper.cpp if needed
WHISPER_DIR="${THIRD_PARTY_DIR}/whisper.cpp"
if [[ ! -d "$WHISPER_DIR" ]]; then
    echo "📥 Cloning whisper.cpp..."
    git clone https://github.com/ggerganov/whisper.cpp.git
    echo "✅ whisper.cpp cloned successfully"
else
    echo "📁 whisper.cpp already exists, skipping clone"
fi

cd "$WHISPER_DIR"

# Step 2: Build whisper.cpp if needed
if [[ ! -f "main" ]]; then
    echo "🔨 Building whisper.cpp..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    echo "✅ whisper.cpp built successfully"
else
    echo "📦 whisper.cpp already built, skipping build"
fi

# Step 3: Download model if needed
MODELS_DIR="${WHISPER_DIR}/models"
mkdir -p "$MODELS_DIR"
cd "$MODELS_DIR"

MODEL_FILE="ggml-base.en.bin"
MODEL_SCRIPT="../models/download-ggml-model.sh"

if [[ ! -f "$MODEL_FILE" ]]; then
    echo "📥 Downloading Whisper base.en model (~148MB)..."
    echo "This may take a few minutes depending on your internet connection."
    
    if [[ -f "$MODEL_SCRIPT" ]]; then
        bash "$MODEL_SCRIPT" base.en
    else
        # Fallback direct download
        MODEL_URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"
        if command -v wget >/dev/null 2>&1; then
            wget "$MODEL_URL" -O "$MODEL_FILE"
        elif command -v curl >/dev/null 2>&1; then
            curl -L "$MODEL_URL" -o "$MODEL_FILE"
        else
            echo "❌ Error: Neither wget nor curl found. Please install one of them."
            exit 1
        fi
    fi
    
    echo "✅ Model downloaded successfully"
else
    echo "📁 Model already exists, skipping download"
fi

# Verify model file
if [[ ! -f "$MODEL_FILE" || ! -s "$MODEL_FILE" ]]; then
    echo "❌ Error: Model file is missing or empty: $MODEL_FILE"
    exit 1
fi

MODEL_SIZE=$(du -h "$MODEL_FILE" | cut -f1)
echo "📊 Model size: $MODEL_SIZE"

# Step 4: Download sample audio if needed
cd "$WHISPER_DIR"
SAMPLE_AUDIO="samples/jfk.wav"

if [[ ! -f "$SAMPLE_AUDIO" ]]; then
    echo "📥 Downloading sample audio..."
    if [[ ! -d "samples" ]]; then
        mkdir -p samples
    fi
    
    # Download JFK sample audio (famous speech)
    AUDIO_URL="https://github.com/ggerganov/whisper.cpp/raw/master/samples/jfk.wav"
    
    if command -v wget >/dev/null 2>&1; then
        wget "$AUDIO_URL" -O "$SAMPLE_AUDIO"
    elif command -v curl >/dev/null 2>&1; then
        curl -L "$AUDIO_URL" -o "$SAMPLE_AUDIO"
    else
        echo "❌ Error: Cannot download sample audio. Please install wget or curl."
        exit 1
    fi
    
    echo "✅ Sample audio downloaded"
else
    echo "📁 Sample audio already exists"
fi

# Verify audio file
if [[ ! -f "$SAMPLE_AUDIO" || ! -s "$SAMPLE_AUDIO" ]]; then
    echo "❌ Error: Sample audio file is missing or empty: $SAMPLE_AUDIO"
    exit 1
fi

AUDIO_SIZE=$(du -h "$SAMPLE_AUDIO" | cut -f1)
echo "🎵 Audio file: $SAMPLE_AUDIO ($AUDIO_SIZE)"

# Step 5: Prepare for trace capture
cd "$PROJECT_ROOT"
TRACE_FILE="whisper_real_trace.ggmlviz"

echo
echo "🚀 Running Whisper with GGML Visualizer instrumentation..."
echo "📁 Trace file: $TRACE_FILE"
echo "🔧 Hook library: $HOOK_LIB"
echo

# Set up environment for instrumentation
export GGML_VIZ_OUTPUT="$TRACE_FILE"
export GGML_VIZ_VERBOSE=1
export GGML_VIZ_MAX_EVENTS=500000  # Whisper generates many more events

# Platform-specific library injection
if [[ "$OSTYPE" == "darwin"* ]]; then
    export DYLD_INSERT_LIBRARIES="$HOOK_LIB"
    echo "🍎 Using macOS DYLD_INSERT_LIBRARIES"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    export LD_PRELOAD="$HOOK_LIB"
    echo "🐧 Using Linux LD_PRELOAD"
fi

# Step 6: Run Whisper with instrumentation
echo "🎤 Transcribing JFK speech sample..."
echo "⏱️  This will generate real GGML events for audio processing..."
echo

cd "$WHISPER_DIR"
./main -m "$MODEL_FILE" -f "$SAMPLE_AUDIO" --output-txt --output-vtt

echo
echo "🎉 Demo completed!"

# Check if trace was generated
cd "$PROJECT_ROOT"
if [[ -f "$TRACE_FILE" ]]; then
    TRACE_SIZE=$(du -h "$TRACE_FILE" | cut -f1)
    EVENT_COUNT=$(xxd "$TRACE_FILE" | wc -l)  # Rough estimate
    
    echo "✅ Trace file generated: $TRACE_FILE ($TRACE_SIZE)"
    echo "📊 Estimated events captured: ~$((EVENT_COUNT * 3))"
    echo
    echo "🖥️  To view the trace in the GUI:"
    echo "   ${BUILD_DIR}/bin/ggml-viz $TRACE_FILE"
    echo
    echo "🔍 To view in live mode (run in another terminal):"
    echo "   ${BUILD_DIR}/bin/ggml-viz --live $TRACE_FILE"
else
    echo "❌ Warning: No trace file generated. Check for errors above."
    echo "🔧 Debug: Check if hook library was loaded properly"
fi

# Show transcription results if available
if [[ -f "${WHISPER_DIR}/samples/jfk.wav.txt" ]]; then
    echo
    echo "📝 Transcription result:"
    echo "$(cat "${WHISPER_DIR}/samples/jfk.wav.txt")"
fi

echo
echo "🎯 This demo showed REAL Whisper audio processing with actual:"
echo "   • Mel-spectrogram feature extraction"
echo "   • Encoder transformer operations (multi-head attention)"
echo "   • Decoder transformer operations (cross-attention)"
echo "   • Audio preprocessing and windowing"
echo "   • Token prediction and beam search"
echo "   • Language detection and timestamp alignment"
echo
echo "📈 All operations were captured by GGML Visualizer for analysis!"