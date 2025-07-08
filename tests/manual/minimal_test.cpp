#include <iostream>
#include <cstdlib>

extern "C" {
    void ggml_viz_hook_graph_compute_begin(const void* graph, const void* backend);
}

int main() {
    std::cout << "Minimal test starting..." << std::endl;
    
    std::cout << "About to call hook (no auto-init)..." << std::endl;
    ggml_viz_hook_graph_compute_begin(nullptr, nullptr);
    std::cout << "Hook called successfully!" << std::endl;
    
    return 0;
}