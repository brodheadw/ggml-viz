/*
 * Whisper Demo for GGML Visualizer
 * 
 * This demo shows how to integrate GGML Visualizer with Whisper-like operations.
 * It demonstrates audio processing, encoder-decoder architecture, and attention mechanisms.
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <memory>
#include <vector>
#include <random>

// GGML Visualizer includes
#include "utils/config.hpp"
#include "utils/logger.hpp"
#include "instrumentation/ggml_hook.hpp"

// GGML includes for creating test computations
#include "ggml.h"

// Simulate Whisper encoder operations
void simulate_whisper_encoder(const std::string& model_name, int mel_length, int n_mels, int layer) {
    auto& hook = ggml_viz::GGMLHook::instance();
    
    // Encoder has convolutional layers and transformer blocks
    int n_nodes = 20 + (layer * 4); // More operations than LLaMA due to conv layers
    
    std::cout << "   🎙️  Encoder Layer " << layer << " - " << n_nodes << " operations\n";
    
    // Simulate graph computation begin
    hook.on_graph_compute_begin(nullptr, nullptr);
    
    // Simulate Whisper encoder operations
    std::vector<std::string> encoder_ops = {
        "mel_spectrogram_input", "conv1d_1", "gelu_1", "conv1d_2", "gelu_2",
        "positional_encoding", "layer_norm_1", "multi_head_attention",
        "attention_dropout", "residual_add_1", "layer_norm_2", 
        "ff_linear_1", "gelu_ff", "ff_dropout", "ff_linear_2",
        "residual_add_2", "cross_attention_prep", "key_value_cache"
    };
    
    for (size_t i = 0; i < encoder_ops.size(); ++i) {
        // Simulate operation timing
        hook.on_op_compute_begin(nullptr, nullptr);
        
        // Audio processing operations tend to be more compute intensive
        int compute_time = 2 + (i % 8); // 2-9ms, longer for conv and attention
        if (encoder_ops[i].find("conv") != std::string::npos) {
            compute_time += 3; // Conv layers take longer
        }
        if (encoder_ops[i].find("attention") != std::string::npos) {
            compute_time += 5; // Attention is most expensive
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(compute_time));
        
        hook.on_op_compute_end(nullptr, nullptr);
        
        std::cout << "     🔊 " << encoder_ops[i] << " (" << compute_time << "ms)\n";
    }
    
    // Simulate graph computation end
    hook.on_graph_compute_end(nullptr, nullptr);
}

// Simulate Whisper decoder operations
void simulate_whisper_decoder(const std::string& model_name, int sequence_length, int vocab_size, int layer) {
    auto& hook = ggml_viz::GGMLHook::instance();
    
    // Decoder has cross-attention with encoder
    int n_nodes = 18 + (layer * 3);
    
    std::cout << "   📝 Decoder Layer " << layer << " - " << n_nodes << " operations\n";
    
    // Simulate graph computation begin
    hook.on_graph_compute_begin(nullptr, nullptr);
    
    // Simulate Whisper decoder operations
    std::vector<std::string> decoder_ops = {
        "token_embeddings", "positional_encoding", "layer_norm_1",
        "masked_self_attention", "attention_dropout", "residual_add_1",
        "layer_norm_2", "cross_attention_query", "cross_attention_key_value",
        "cross_attention_scores", "cross_attention_softmax", "cross_attention_out",
        "cross_attention_dropout", "residual_add_2", "layer_norm_3",
        "ff_linear_1", "gelu_ff", "ff_linear_2", "residual_add_3"
    };
    
    for (size_t i = 0; i < decoder_ops.size(); ++i) {
        // Simulate operation timing
        hook.on_op_compute_begin(nullptr, nullptr);
        
        // Decoder operations, cross-attention is expensive
        int compute_time = 1 + (i % 6); // 1-6ms
        if (decoder_ops[i].find("cross_attention") != std::string::npos) {
            compute_time += 4; // Cross-attention with encoder is expensive
        }
        if (decoder_ops[i].find("masked") != std::string::npos) {
            compute_time += 2; // Masked attention has additional complexity
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(compute_time));
        
        hook.on_op_compute_end(nullptr, nullptr);
        
        std::cout << "     ✍️  " << decoder_ops[i] << " (" << compute_time << "ms)\n";
    }
    
    // Simulate graph computation end
    hook.on_graph_compute_end(nullptr, nullptr);
}

// Simulate audio preprocessing operations
void simulate_audio_preprocessing(const std::string& audio_type, int sample_rate, float duration_sec) {
    auto& hook = ggml_viz::GGMLHook::instance();
    
    std::cout << "   🎵 Audio Preprocessing - " << audio_type << " (" << duration_sec << "s @ " << sample_rate << "Hz)\n";
    
    // Simulate graph computation begin
    hook.on_graph_compute_begin(nullptr, nullptr);
    
    // Audio preprocessing pipeline
    std::vector<std::string> preprocessing_ops = {
        "audio_load", "resampling", "windowing", "fft_transform",
        "mel_filterbank", "log_mel", "normalization", "padding"
    };
    
    for (size_t i = 0; i < preprocessing_ops.size(); ++i) {
        hook.on_op_compute_begin(nullptr, nullptr);
        
        // Audio preprocessing timing varies by operation
        int compute_time = 1 + (i % 4);
        if (preprocessing_ops[i] == "fft_transform") {
            compute_time += 8; // FFT is computationally expensive
        }
        if (preprocessing_ops[i] == "mel_filterbank") {
            compute_time += 4; // Mel filterbank application
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(compute_time));
        
        hook.on_op_compute_end(nullptr, nullptr);
        
        std::cout << "     🎛️  " << preprocessing_ops[i] << " (" << compute_time << "ms)\n";
    }
    
    hook.on_graph_compute_end(nullptr, nullptr);
}

int main(int argc, char* argv[]) {
    std::cout << "🎙️ GGML Visualizer - Whisper Demo (Config-Driven)\n\n";
    
    try {
        // 1. Load configuration
        auto& config_mgr = ggml_viz::ConfigManager::instance();
        
        // Determine config file path
        std::string config_file = "whisper_demo_config.json";
        if (argc > 1) {
            config_file = argv[1];
        }
        
        // Check if config file exists
        if (!std::filesystem::exists(config_file)) {
            std::cerr << "❌ Error: Config file not found: " << config_file << "\n";
            std::cerr << "Usage: " << argv[0] << " [config_file.json]\n";
            std::cerr << "Default config: whisper_demo_config.json\n";
            return 1;
        }
        
        // Load configuration with demo config precedence
        config_mgr.load_with_precedence(config_file, "", "");
        auto config = config_mgr.get();
        
        // Configure logger
        ggml_viz::Logger::instance().configure_from_config(*config);
        
        std::cout << "📋 Loaded configuration from: " << config_file << "\n";
        std::cout << "📊 Max events: " << config->instrumentation.max_events << "\n";
        std::cout << "📁 Output file: " << config->output.filename << "\n";
        std::cout << "🎯 Op timing: " << (config->instrumentation.enable_op_timing ? "enabled" : "disabled") << "\n";
        std::cout << "💾 Memory tracking: " << (config->instrumentation.enable_memory_tracking ? "enabled" : "disabled") << "\n";
        std::cout << "🏷️  Tensor names: " << (config->instrumentation.record_tensor_names ? "enabled" : "disabled") << "\n\n";
        
        // 2. Initialize GGML hooks with config
        auto& hook = ggml_viz::GGMLHook::instance();
        
        std::cout << "🔧 Starting GGML instrumentation...\n";
        hook.start();
        
        if (!hook.is_active()) {
            std::cerr << "❌ Error: Failed to start GGML hooks\n";
            return 1;
        }
        
        std::cout << "✅ GGML hooks active\n\n";
        
        // 3. Simulate Whisper-like workloads
        std::cout << "🚀 Starting Whisper-like computation simulation...\n";
        std::cout << "📊 Watch real-time events in: " << config->output.filename << "\n";
        std::cout << "🎥 Open visualizer: ./ggml-viz --live " << config->output.filename << "\n\n";
        
        // Different Whisper model configurations
        struct WhisperModelConfig {
            std::string name;
            int n_mels;         // Mel spectrogram features
            int encoder_layers;
            int decoder_layers;
            int vocab_size;
        };
        
        std::vector<WhisperModelConfig> models = {
            {"Whisper-tiny", 80, 4, 4, 51864},
            {"Whisper-base", 80, 6, 6, 51864},
            {"Whisper-small", 80, 12, 12, 51864}
        };
        
        // Different audio scenarios
        struct AudioScenario {
            std::string type;
            int sample_rate;
            float duration;
            std::string language;
        };
        
        std::vector<AudioScenario> scenarios = {
            {"podcast", 16000, 30.0f, "english"},
            {"meeting", 16000, 15.0f, "english"},
            {"music", 16000, 10.0f, "multilingual"}
        };
        
        for (const auto& scenario : scenarios) {
            std::cout << "🎧 Processing " << scenario.type << " audio scenario\n";
            std::cout << "   🌍 Language: " << scenario.language << "\n";
            std::cout << "   ⏱️  Duration: " << scenario.duration << " seconds\n";
            
            // 1. Audio preprocessing
            simulate_audio_preprocessing(scenario.type, scenario.sample_rate, scenario.duration);
            std::cout << "   ✅ Audio preprocessing completed\n";
            std::cout << "   📈 Events so far: " << hook.event_count() << "\n";
            
            // 2. Process with different model sizes
            for (const auto& model_config : models) {
                std::cout << "\n🤖 Processing with " << model_config.name << " model\n";
                std::cout << "   📏 Mel features: " << model_config.n_mels << "\n";
                std::cout << "   🔗 Encoder layers: " << model_config.encoder_layers << "\n";
                std::cout << "   🔗 Decoder layers: " << model_config.decoder_layers << "\n";
                
                // Encoder phase
                std::cout << "   🎙️  Running encoder...\n";
                int encoder_layers_to_sim = std::min(model_config.encoder_layers, 3);
                
                for (int layer = 0; layer < encoder_layers_to_sim; ++layer) {
                    simulate_whisper_encoder(model_config.name, 
                                           static_cast<int>(scenario.duration * scenario.sample_rate / 160), // Mel length
                                           model_config.n_mels, 
                                           layer);
                    
                    std::cout << "     ✅ Encoder layer " << (layer + 1) << " completed\n";
                    
                    // Small delay for real-time visualization
                    std::this_thread::sleep_for(std::chrono::milliseconds(150));
                }
                
                std::cout << "   ✅ Encoder phase completed\n";
                
                // Decoder phase (simulate generating tokens)
                std::cout << "   📝 Running decoder...\n";
                int decoder_layers_to_sim = std::min(model_config.decoder_layers, 3);
                int max_tokens = static_cast<int>(scenario.duration * 3); // ~3 tokens per second
                
                for (int token = 0; token < std::min(max_tokens, 20); ++token) { // Limit for demo
                    for (int layer = 0; layer < decoder_layers_to_sim; ++layer) {
                        simulate_whisper_decoder(model_config.name, 
                                               token + 1, // Current sequence length
                                               model_config.vocab_size, 
                                               layer);
                    }
                    
                    if (token % 5 == 0) {
                        std::cout << "     📊 Generated " << (token + 1) << " tokens, events: " << hook.event_count() << "\n";
                    }
                    
                    // Very short delay between tokens
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                
                std::cout << "   ✅ Decoder phase completed\n";
                std::cout << "   📊 Total events: " << hook.event_count() << "\n";
                
                // Brief pause between models
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
            
            std::cout << "✅ " << scenario.type << " scenario completed\n\n";
            
            // Pause between scenarios
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // 4. Additional specialized Whisper operations
        std::cout << "🔄 Running additional Whisper-specific operations...\n";
        
        // Voice Activity Detection simulation
        std::cout << "   🔍 Voice Activity Detection (VAD)\n";
        for (int i = 0; i < 5; ++i) {
            hook.on_graph_compute_begin(nullptr, nullptr);
            hook.on_op_compute_begin(nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            hook.on_op_compute_end(nullptr, nullptr);
            hook.on_graph_compute_end(nullptr, nullptr);
            std::cout << "     🎚️  VAD segment " << (i + 1) << "\n";
        }
        
        // Language detection simulation
        std::cout << "   🌐 Language Detection\n";
        std::vector<std::string> languages = {"english", "spanish", "french", "german", "chinese"};
        for (const auto& language : languages) {
            hook.on_graph_compute_begin(nullptr, nullptr);
            hook.on_op_compute_begin(nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            hook.on_op_compute_end(nullptr, nullptr);
            hook.on_graph_compute_end(nullptr, nullptr);
            std::cout << "     🗣️  " << language << " probability calculated\n";
        }
        
        // 5. Summary and cleanup
        std::cout << "\n🎯 Demo completed successfully!\n";
        std::cout << "📊 Total events captured: " << hook.event_count() << "\n";
        std::cout << "💾 Trace file: " << config->output.filename << "\n\n";
        
        std::cout << "🔍 To visualize the captured data:\n";
        if (config->ui.live_mode) {
            std::cout << "   ./ggml-viz --live " << config->output.filename << "\n";
        } else {
            std::cout << "   ./ggml-viz " << config->output.filename << "\n";
        }
        std::cout << "\n";
        
        // Cleanup
        hook.stop();
        
        std::cout << "✨ Whisper Demo completed successfully!\n";
        std::cout << "💡 This demo simulated Whisper-like operations for speech recognition\n";
        std::cout << "💡 The trace shows encoder-decoder architecture with cross-attention\n";
        std::cout << "💡 Audio preprocessing, VAD, and language detection were demonstrated\n";
        std::cout << "💡 All operations were captured using your configured settings:\n";
        std::cout << "   📋 Config file: " << config_file << "\n";
        std::cout << "   📊 Max events: " << config->instrumentation.max_events << "\n";
        std::cout << "   🎯 Op timing: " << (config->instrumentation.enable_op_timing ? "✅" : "❌") << "\n";
        std::cout << "   💾 Memory tracking: " << (config->instrumentation.enable_memory_tracking ? "✅" : "❌") << "\n";
        std::cout << "   🏷️  Tensor names: " << (config->instrumentation.record_tensor_names ? "✅" : "❌") << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
}