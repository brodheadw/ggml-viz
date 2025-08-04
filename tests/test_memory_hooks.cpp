#include "ggml_hook.hpp"
#include "../src/utils/config.hpp"
#include "ggml.h"
#include "ggml-alloc.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== Memory Hook Test ===" << std::endl;
    
    // Initialize configuration with memory tracking enabled
    auto& config_mgr = ggml_viz::ConfigManager::instance();
    
    // Create a temporary config with memory tracking enabled
    std::cout << "Loading test configuration..." << std::endl;
    try {
        config_mgr.load_with_precedence("", "../memory_test_config.json", "");
    } catch (const std::exception& e) {
        std::cout << "Could not load config file, using defaults with memory tracking enabled" << std::endl;
        // Fallback to environment variables
        setenv("GGML_VIZ_OUTPUT", "memory_test_trace.ggmlviz", 1);
        setenv("GGML_VIZ_MEMORY_TRACKING", "true", 1);
        setenv("GGML_VIZ_VERBOSE", "true", 1);
        config_mgr.load_with_precedence("", "", "");
    }
    
    auto config = config_mgr.get();
    std::cout << "Memory tracking enabled: " << (config->instrumentation.enable_memory_tracking ? "yes" : "no") << std::endl;
    
    // Start hooks
    auto& hook = ggml_viz::GGMLHook::instance();
    hook.start();
    
    std::cout << "Hook started, active: " << hook.is_active() << std::endl;
    
    // Test manual memory event recording
    std::cout << "Testing manual memory event recording..." << std::endl;
    
    // Create a simple GGML context 
    struct ggml_init_params params = {
        .mem_size = 16 * 1024, // 16KB - small for testing
        .mem_buffer = nullptr,
        .no_alloc = false
    };
    
    struct ggml_context* ctx = ggml_init(params);
    if (!ctx) {
        std::cerr << "Failed to create GGML context" << std::endl;
        return 1;
    }
    
    std::cout << "Created GGML context" << std::endl;
    
    // Create some tensors
    struct ggml_tensor* a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 4);
    struct ggml_tensor* b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 4);
    
    if (a && b) {
        std::cout << "Created tensors:" << std::endl;
        std::cout << "  a: " << ggml_nbytes(a) << " bytes" << std::endl;
        std::cout << "  b: " << ggml_nbytes(b) << " bytes" << std::endl;
        
        // Manually trigger memory allocation events for testing
        if (hook.is_active()) {
            hook.on_tensor_alloc(a, ggml_nbytes(a), nullptr);
            hook.on_tensor_alloc(b, ggml_nbytes(b), nullptr);
            std::cout << "Manually triggered memory allocation events" << std::endl;
            
            // Test memory free events
            hook.on_tensor_free(a, nullptr);
            hook.on_tensor_free(b, nullptr);
            std::cout << "Manually triggered memory free events" << std::endl;
        }
    }
    
    // Check how many events we recorded
    size_t event_count = hook.event_count();
    std::cout << "Recorded " << event_count << " events" << std::endl;
    std::cout << "Dropped " << hook.get_dropped_events() << " events" << std::endl;
    
    // Verify we have some memory events
    auto events = hook.consume_available_events();
    size_t memory_events = 0;
    for (const auto& event : events) {
        if (event.type == ggml_viz::EventType::TENSOR_ALLOC || 
            event.type == ggml_viz::EventType::TENSOR_FREE) {
            memory_events++;
            std::cout << "Found memory event: " << 
                (event.type == ggml_viz::EventType::TENSOR_ALLOC ? "ALLOC" : "FREE") <<
                ", size: " << event.data.memory.size << " bytes" << std::endl;
        }
    }
    
    std::cout << "Memory events found: " << memory_events << std::endl;
    
    // Clean up
    ggml_free(ctx);
    hook.stop();
    
    // Test success criteria
    bool success = (event_count > 0) && (memory_events > 0);
    
    if (success) {
        std::cout << "✓ Memory hook test PASSED" << std::endl;
        std::cout << "Check memory_test_trace.ggmlviz for trace file" << std::endl;
    } else {
        std::cout << "✗ Memory hook test FAILED" << std::endl;
        std::cout << "Expected: event_count > 0 and memory_events > 0" << std::endl;
        std::cout << "Actual: event_count = " << event_count << ", memory_events = " << memory_events << std::endl;
    }
    
    return success ? 0 : 1;
}