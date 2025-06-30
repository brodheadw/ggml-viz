#include <stdio.h>
#include <ggml.h>

int main() {
    printf("Creating minimal GGML operations...\n");
    
    // Initialize GGML context
    struct ggml_init_params params = {
        .mem_size   = 16*1024*1024,  // 16 MB
        .mem_buffer = NULL,
        .no_alloc   = false,
    };
    
    struct ggml_context* ctx = ggml_init(params);
    if (!ctx) {
        printf("Failed to initialize GGML context\n");
        return 1;
    }
    
    printf("GGML context initialized\n");
    
    // Create some tensors
    struct ggml_tensor* a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 4);
    struct ggml_tensor* b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 4);
    
    printf("Tensors created: a=%p, b=%p\n", (void*)a, (void*)b);
    
    // Perform some operations
    struct ggml_tensor* sum = ggml_add(ctx, a, b);
    struct ggml_tensor* mul = ggml_mul(ctx, a, b);
    
    printf("Operations created: sum=%p, mul=%p\n", (void*)sum, (void*)mul);
    
    // Build computation graph
    struct ggml_cgraph* gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, sum);
    ggml_build_forward_expand(gf, mul);
    
    printf("Graph built with %d nodes\n", gf->n_nodes);
    
    // This should trigger our hooks if injection is working
    ggml_graph_compute_with_ctx(ctx, gf, 1);
    
    printf("Graph computation complete\n");
    
    ggml_free(ctx);
    printf("GGML context freed\n");
    
    return 0;
}
