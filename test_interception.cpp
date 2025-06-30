// test_interception.cpp - Direct test of function interception
#include <iostream>

// Declare the functions we're trying to intercept
extern "C" {
    struct ggml_cgraph {
        int n_nodes;
        void* nodes[1024];  // Simplified for testing
    };
    
    // These should be intercepted by our shared library
    enum ggml_status { GGML_STATUS_SUCCESS = 0, GGML_STATUS_FAILED = 1 };
    
    enum ggml_status ggml_backend_graph_compute(void* backend, struct ggml_cgraph* cgraph);
}

int main() {
    std::cout << "=== Direct Function Interception Test ===" << std::endl;
    
    // Create a dummy graph
    struct ggml_cgraph dummy_graph;
    dummy_graph.n_nodes = 3;  // Pretend we have 3 nodes
    
    std::cout << "Creating dummy graph with " << dummy_graph.n_nodes << " nodes" << std::endl;
    
    std::cout << "*** Calling ggml_backend_graph_compute directly ***" << std::endl;
    std::cout << "This should trigger our function interception!" << std::endl;
    
    // Call the function that should be intercepted
    enum ggml_status result = ggml_backend_graph_compute(nullptr, &dummy_graph);
    
    std::cout << "Function returned with status: " << (int)result << std::endl;
    
    if (result == GGML_STATUS_SUCCESS) {
        std::cout << "✅ Function call completed successfully!" << std::endl;
    } else {
        std::cout << "⚠️ Function call returned error status" << std::endl;
    }
    
    std::cout << "Test completed!" << std::endl;
    return 0;
}