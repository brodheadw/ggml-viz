// tests/test_ggml_hook.cpp
#include "ggml_hook.hpp"
#include "utils/config.hpp"
#include "ggml-cpu.h"
#include "ggml-impl.h"
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    // Initialize ggml
    size_t mem_size = 128 * 1024 * 1024; // 128 MB
    void* mem_buffer = malloc(mem_size);

    struct ggml_init_params params = {};
    params.mem_size = mem_size;
    params.mem_buffer = mem_buffer;
    params.no_alloc = false;

    struct ggml_context* ctx = ggml_init(params);

    // Configure using environment variables for simplicity in tests
    putenv(const_cast<char*>("GGML_VIZ_OUTPUT=test_trace.ggmlviz"));
    putenv(const_cast<char*>("GGML_VIZ_VERBOSE=1"));

    // Initialize ConfigManager with environment variables
    ggml_viz::ConfigManager& config_mgr = ggml_viz::ConfigManager::instance();
    config_mgr.load_with_precedence("", "", "");  // Empty paths - will use env vars and defaults

    ggml_viz::GGMLHook::instance().start();

    std::cout << "Hook started, active: " << ggml_viz::GGMLHook::instance().is_active() << "\n";
    std::cout << "Creating computation graph...\n";

    // Create a simple computation graph
    int n = 1024;
    struct ggml_tensor* A = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, n, n);
    struct ggml_tensor* B = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, n, n);
    struct ggml_tensor* C = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, n, n);

    ggml_set_name(A, "matrix_A");
    ggml_set_name(B, "matrix_B");
    ggml_set_name(C, "matrix_C");

    float* a_data = static_cast<float*>(A->data);
    float* b_data = static_cast<float*>(B->data);
    float* c_data = static_cast<float*>(C->data);

    for (int i=0; i<n*n; i++) {
        a_data[i] = rand() / (float)RAND_MAX;
        b_data[i] = rand() / (float)RAND_MAX;
        c_data[i] = rand() / (float)RAND_MAX;
    }

    struct ggml_tensor* AB = ggml_mul_mat(ctx, A, B);
    ggml_set_name(AB, "matmul_AB");

    struct ggml_tensor* result = ggml_add(ctx, AB, C);
    ggml_set_name(result, "final_result");

    struct ggml_cgraph* gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, result);

    std::cout << "Graph has " << gf->n_nodes << " nodes\n";

    std::cout << "Running computation...\n";
    std::cout << "Hook active before computation: " << ggml_viz::GGMLHook::instance().is_active() << "\n";
    auto start = std::chrono::high_resolution_clock::now();

    for (int i=0; i<10; i++) {
        // Manually call hooks around computation since function interception is disabled in test mode
        ggml_viz::GGMLHook::instance().on_graph_compute_begin(gf, nullptr);
        
        // Call hooks for each operation
        for (int j = 0; j < gf->n_nodes; j++) {
            if (gf->nodes[j]) {
                ggml_viz::GGMLHook::instance().on_op_compute_begin(gf->nodes[j], nullptr);
            }
        }
        
        // Actual computation
        ggml_graph_compute_with_ctx(ctx, gf, 4);
        
        // End hooks for each operation
        for (int j = 0; j < gf->n_nodes; j++) {
            if (gf->nodes[j]) {
                ggml_viz::GGMLHook::instance().on_op_compute_end(gf->nodes[j], nullptr);
            }
        }
        
        ggml_viz::GGMLHook::instance().on_graph_compute_end(gf, nullptr);
        std::cout << " Iteration " << i+1 << " complete\n";
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Total time: " << duration.count() << " ms\n";

    // Stop hooks and flush data
    ggml_viz::GGMLHook::instance().stop();

    std::cout << "\nTrace complete!\n";
    std::cout << "Events recorded: " << ggml_viz::GGMLHook::instance().event_count() << "\n";
    std::cout << "Trace file: test_trace.ggmlviz\n";

    ggml_graph_dump_dot(gf, NULL, "test_graph.dot");
    std::cout << "Graph structure: test_graph.dot\n";

    ggml_free(ctx);
    free(mem_buffer);

    return 0;
}