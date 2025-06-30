// simple_ggml_test.cpp - Simple test to trigger function interception
#include <iostream>

// Include GGML headers
extern "C" {
    #include "ggml.h"
}

int main() {
    std::cout << "=== Simple GGML Function Test ===" << std::endl;
    
    try {
        // Initialize GGML context  
        struct ggml_init_params params = {0};
        params.mem_size = 1024 * 1024; // 1MB
        params.mem_buffer = nullptr;
        params.no_alloc = false;
        
        std::cout << "Creating GGML context..." << std::endl;
        struct ggml_context* ctx = ggml_init(params);
        if (!ctx) {
            std::cerr << "Failed to create GGML context" << std::endl;
            return 1;
        }
        
        std::cout << "Creating a simple tensor..." << std::endl;
        
        // Create a simple tensor
        struct ggml_tensor* tensor = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 10);
        if (!tensor) {
            std::cerr << "Failed to create tensor" << std::endl;
            ggml_free(ctx);
            return 1;
        }
        
        // Set a name for better tracing
        ggml_set_name(tensor, "test_tensor");
        
        std::cout << "Tensor created successfully!" << std::endl;
        std::cout << "Tensor name: " << (tensor->name ? tensor->name : "unnamed") << std::endl;
        
        // Try to create a computation graph (if functions are available)
        std::cout << "Creating computation graph..." << std::endl;
        struct ggml_cgraph* graph = ggml_new_graph(ctx);
        if (graph) {
            std::cout << "Graph created successfully!" << std::endl;
            
            // Add tensor to graph
            ggml_build_forward_expand(graph, tensor);
            std::cout << "Added tensor to graph" << std::endl;
        } else {
            std::cout << "Graph creation failed or not available" << std::endl;
        }
        
        // Cleanup
        std::cout << "Cleaning up..." << std::endl;
        ggml_free(ctx);
        
        std::cout << "Test completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
    
    return 0;
}