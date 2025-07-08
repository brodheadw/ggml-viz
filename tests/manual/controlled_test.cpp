#include <iostream>
#include <cstdlib>

extern "C" {
    void ggml_viz_hook_graph_compute_begin(const void* graph, const void* backend);
}

int main() {
    std::cout << "Controlled test - no environment variables..." << std::endl;
    
    // Explicitly unset the environment variable
    unsetenv("GGML_VIZ_OUTPUT");
    
    std::cout << "Calling hook without auto-initialization..." << std::endl;
    ggml_viz_hook_graph_compute_begin(nullptr, nullptr);
    std::cout << "First call successful!" << std::endl;
    
    // Now set the environment variable and try again
    std::cout << "Setting GGML_VIZ_OUTPUT and trying again..." << std::endl;
    setenv("GGML_VIZ_OUTPUT", "controlled_test.ggmlviz", 1);
    
    std::cout << "Calling hook with auto-initialization..." << std::endl;
    ggml_viz_hook_graph_compute_begin(nullptr, nullptr);
    std::cout << "Second call successful!" << std::endl;
    
    return 0;
}