// simple_ggml_compute.cpp - Minimal GGML program that actually calls compute functions
#include <stdio.h>
#include <stdlib.h>
#include <ggml.h>
#include <ggml-cpu.h>

int main() {
    printf("=== Simple GGML Compute Test ===\n");
    
    // Initialize GGML context
    struct ggml_init_params params = {
        .mem_size   = 16*1024*1024,  // 16 MB
        .mem_buffer = NULL,
        .no_alloc   = false,
    };
    
    struct ggml_context* ctx = ggml_init(params);
    if (!ctx) {
        printf("❌ Failed to initialize GGML context\n");
        return 1;
    }
    
    printf("✅ GGML context initialized\n");
    
    // Create some simple tensors
    struct ggml_tensor* a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 4);
    struct ggml_tensor* b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 4);
    
    // Fill with some values
    for (int i = 0; i < 16; i++) {
        ((float*)a->data)[i] = (float)i;
        ((float*)b->data)[i] = (float)(i + 1);
    }
    
    printf("✅ Created and filled tensors\n");
    
    // Create operations
    struct ggml_tensor* sum = ggml_add(ctx, a, b);
    printf("✅ Created add operation\n");
    
    // Build computation graph
    struct ggml_cgraph* gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, sum);
    
    printf("✅ Built computation graph\n");
    printf("🔥 About to call ggml_graph_compute_with_ctx - THIS SHOULD TRIGGER OUR HOOKS!\n");
    
    // This should trigger our function override!
    ggml_graph_compute_with_ctx(ctx, gf, 1);
    
    printf("✅ Graph computation completed\n");
    
    // Print result
    printf("📊 Result: a[0] + b[0] = %.2f\n", ((float*)sum->data)[0]);
    
    ggml_free(ctx);
    printf("✅ GGML context freed\n");
    
    return 0;
}