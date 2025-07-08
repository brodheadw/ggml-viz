#include <stdio.h>
#include <dlfcn.h>

// Declare the hook functions
extern void ggml_viz_hook_graph_compute_begin(const void* graph, const void* backend);

int main() {
    // Get address of the function in main executable
    void* main_addr = (void*)ggml_viz_hook_graph_compute_begin;
    
    // Try to get address from our injected library
    void* lib_addr = dlsym(RTLD_DEFAULT, "ggml_viz_hook_graph_compute_begin");
    
    printf("Main executable function address: %p\n", main_addr);
    printf("Dynamic library function address: %p\n", lib_addr);
    printf("Addresses match: %s\n", (main_addr == lib_addr) ? "YES (injection working)" : "NO (injection failed)");
    
    // Call the function to see what happens
    printf("Calling hook function...\n");
    ggml_viz_hook_graph_compute_begin(NULL, NULL);
    printf("Hook function returned.\n");
    
    return 0;
}