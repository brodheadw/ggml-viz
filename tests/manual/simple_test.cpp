#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "Setting environment..." << std::endl;
    setenv("GGML_VIZ_OUTPUT", "simple_test.ggmlviz", 1);
    
    std::cout << "About to declare hook function..." << std::endl;
    
    // Just declare the function, don't call it yet
    extern "C" void ggml_viz_hook_graph_compute_begin(const void* graph, const void* backend);
    
    std::cout << "Function declared. About to call it..." << std::endl;
    
    // Now call it
    ggml_viz_hook_graph_compute_begin(nullptr, nullptr);
    
    std::cout << "Function called successfully!" << std::endl;
    
    return 0;
}