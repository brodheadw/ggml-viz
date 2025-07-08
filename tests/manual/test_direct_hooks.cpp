#include <iostream>
#include <cstdlib>

// Test if our hooks are properly linked
extern "C" {
    void ggml_viz_hook_graph_compute_begin(const void* graph, const void* backend);
    void ggml_viz_hook_graph_compute_end(const void* graph, const void* backend);
}

int main() {
    // Set up environment for hook testing
    setenv("GGML_VIZ_OUTPUT", "direct_hooks_test.ggmlviz", 1);
    
    std::cout << "Testing direct-linked hooks..." << std::endl;
    
    // Call the hooks directly
    std::cout << "Calling ggml_viz_hook_graph_compute_begin..." << std::endl;
    ggml_viz_hook_graph_compute_begin((void*)0x1234, (void*)0x5678);
    
    std::cout << "Calling ggml_viz_hook_graph_compute_end..." << std::endl;
    ggml_viz_hook_graph_compute_end((void*)0x1234, (void*)0x5678);
    
    std::cout << "Hook calls completed." << std::endl;
    
    return 0;
}