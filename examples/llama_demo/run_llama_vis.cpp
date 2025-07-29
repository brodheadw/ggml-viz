/*
 * LLaMA Demo for GGML Visualizer
 * 
 * This demo shows how to integrate GGML Visualizer with config-driven setup.
 * It demonstrates the configuration system and manual hook triggering.
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

// Simulate LLaMA-like events by manually triggering hooks
void simulate_llama_operations(const std::string& model_name, int sequence_length, int embed_dim, int layer) {
    auto& hook = ggml_viz::GGMLHook::instance();
    
    // Use nullptr for graph pointer since we can't access the full struct
    int n_nodes = 15 + (layer * 3); // Simulate increasing complexity
    
    std::cout << "   🧠 Layer " << layer << " - " << n_nodes << " operations\n";
    
    // Simulate graph computation begin
    hook.on_graph_compute_begin(nullptr, nullptr);
    
    // Simulate individual operations in a transformer layer
    std::vector<std::string> ops = {
        "input_embeddings", "pos_embeddings", "layer_norm_1",
        "query_proj", "key_proj", "value_proj", 
        "attention_scores", "attention_softmax", "attention_out",
        "layer_norm_2", "ff_linear_1", "ff_gelu", "ff_linear_2",
        "residual_add_1", "residual_add_2"
    };
    
    for (size_t i = 0; i < ops.size(); ++i) {
        // Use nullptr for tensor pointer to avoid struct access issues
        
        // Simulate operation timing
        hook.on_op_compute_begin(nullptr, nullptr);
        
        // Simulate computation time based on operation type
        int compute_time = 1 + (i % 5); // 1-5ms
        std::this_thread::sleep_for(std::chrono::milliseconds(compute_time));
        
        hook.on_op_compute_end(nullptr, nullptr);
        
        std::cout << "     ⚡ " << ops[i] << " (" << compute_time << "ms)\n";
    }
    
    // Simulate graph computation end
    hook.on_graph_compute_end(nullptr, nullptr);
}

int main(int argc, char* argv[]) {
    std::cout << "🦙 GGML Visualizer - LLaMA Demo (Config-Driven)\n\n";
    
    try {
        // 1. Load configuration
        auto& config_mgr = ggml_viz::ConfigManager::instance();
        
        // Determine config file path
        std::string config_file = "llama_demo_config.json";
        if (argc > 1) {
            config_file = argv[1];
        }
        
        // Check if config file exists
        if (!std::filesystem::exists(config_file)) {
            std::cerr << "❌ Error: Config file not found: " << config_file << "\n";
            std::cerr << "Usage: " << argv[0] << " [config_file.json]\n";
            std::cerr << "Default config: llama_demo_config.json\n";
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
        
        // 3. Simulate LLaMA-like workloads
        std::cout << "🚀 Starting LLaMA-like computation simulation...\n";
        std::cout << "📊 Watch real-time events in: " << config->output.filename << "\n";
        std::cout << "🎥 Open visualizer: ./ggml-viz --live " << config->output.filename << "\n\n";
        
        // Different model sizes to simulate
        struct ModelConfig {
            std::string name;
            int sequence_length;
            int embed_dim;
            int layers;
        };
        
        std::vector<ModelConfig> models = {
            {"TinyLlama-1.1B", 64, 2048, 22},
            {"Llama-7B-like", 128, 4096, 32},
            {"Llama-13B-like", 256, 5120, 40}
        };
        
        for (const auto& model_config : models) {
            std::cout << "🤖 Simulating " << model_config.name << " model\n";
            std::cout << "   📏 Sequence length: " << model_config.sequence_length << "\n";
            std::cout << "   📐 Embedding dim: " << model_config.embed_dim << "\n";
            std::cout << "   🔗 Layers: " << model_config.layers << "\n";
            
            // Simulate a few layers to keep demo time reasonable
            int layers_to_sim = std::min(model_config.layers, 3);
            
            for (int layer = 0; layer < layers_to_sim; ++layer) {
                simulate_llama_operations(model_config.name, 
                                        model_config.sequence_length, 
                                        model_config.embed_dim, 
                                        layer);
                
                std::cout << "   ✅ Layer " << (layer + 1) << " completed\n";
                std::cout << "   📈 Events so far: " << hook.event_count() << "\n";
                
                // Small delay to see events in real-time
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            
            std::cout << "✅ " << model_config.name << " simulation completed\n\n";
            
            // Pause between models
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // 4. Generate some additional workloads with different patterns
        std::cout << "🔄 Running additional mixed workloads...\n";
        
        // Simulate different inference patterns
        std::vector<std::string> patterns = {"chat", "completion", "embedding"};
        
        for (const auto& pattern : patterns) {
            std::cout << "   🎭 Pattern: " << pattern << "\n";
            
            // Different complexity for different patterns
            int iterations = (pattern == "embedding") ? 5 : 2;
            
            for (int i = 0; i < iterations; ++i) {
                simulate_llama_operations("Mixed-" + pattern, 32, 1024, i);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            std::cout << "   ✅ " << pattern << " pattern completed\n";
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
        
        std::cout << "✨ LLaMA Demo completed successfully!\n";
        std::cout << "💡 This demo simulated LLaMA-like operations using manual hook calls\n";
        std::cout << "💡 The trace shows transformer layers with attention and feed-forward ops\n";
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