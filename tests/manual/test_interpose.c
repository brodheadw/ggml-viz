#include <stdio.h>
#include <dlfcn.h>

// Test function that exists in our program
extern void ggml_viz_hook_graph_compute_begin(const void* graph, const void* backend);

// Our replacement function with debug output
void my_ggml_viz_hook_graph_compute_begin(const void* graph, const void* backend) {
    printf("*** INTERPOSED FUNCTION CALLED! graph=%p backend=%p ***\n", graph, backend);
}

// macOS function interposition structure
__attribute__((used)) static struct { 
    const void* replacement; 
    const void* replacee; 
} _interpose_ggml_viz_hook_graph_compute_begin 
__attribute__ ((section ("__DATA,__interpose"))) = { 
    (const void*)(unsigned long)&my_ggml_viz_hook_graph_compute_begin, 
    (const void*)(unsigned long)&ggml_viz_hook_graph_compute_begin 
};

int main() {
    printf("Testing function interposition...\n");
    
    // Call the function - should be interposed
    ggml_viz_hook_graph_compute_begin((void*)0x1234, (void*)0x5678);
    
    return 0;
}