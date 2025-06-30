// test_ggml_functions.cpp - Test program to trigger GGML function calls
#include <iostream>
#include <vector>

// Include GGML headers
extern "C" {
    #include "ggml.h"
    #include "ggml-backend.h"
}

int main() {
    std::cout << "=== GGML Function Interception Test ===" << std::endl;
    
    try {
        // Initialize GGML context  
        struct ggml_init_params params = {
            .mem_size   = 128 * 1024 * 1024, // 128MB
            .mem_buffer = nullptr,
            .no_alloc   = false,
        };
        
        std::cout << "Creating GGML context..." << std::endl;
        struct ggml_context* ctx = ggml_init(params);
        if (!ctx) {
            std::cerr << "Failed to create GGML context" << std::endl;
            return 1;
        }
        
        std::cout << "Creating tensors..." << std::endl;
        
        // Create some simple tensors
        struct ggml_tensor* a = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 4);
        struct ggml_tensor* b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 4);
        
        // Set names for better tracing
        ggml_set_name(a, "tensor_a");
        ggml_set_name(b, "tensor_b");
        
        std::cout << "Creating computation graph..." << std::endl;
        
        // Create a simple computation: c = a + b
        struct ggml_tensor* c = ggml_add(ctx, a, b);
        ggml_set_name(c, "tensor_c_sum");
        
        // Build computation graph
        struct ggml_cgraph* graph = ggml_new_graph(ctx);
        ggml_build_forward_expand(graph, c);
        
        std::cout << "Graph created with " << graph->n_nodes << " nodes" << std::endl;
        
        // Try to get a CPU backend
        std::cout << "Getting CPU backend..." << std::endl;
        ggml_backend_t backend = ggml_backend_cpu_init();
        if (!backend) {
            std::cerr << "Failed to initialize CPU backend" << std::endl;
            ggml_free(ctx);
            return 1;
        }
        
        std::cout << "*** CALLING ggml_backend_graph_compute - This should be intercepted! ***" << std::endl;
        
        // This should trigger our function interception!
        enum ggml_status status = ggml_backend_graph_compute(backend, graph);
        
        std::cout << "Graph computation completed with status: " << (int)status << std::endl;
        
        // Cleanup
        std::cout << "Cleaning up..." << std::endl;
        ggml_backend_free(backend);
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