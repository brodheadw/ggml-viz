// src/instrumentation/dyld_interpose_simple.cpp
// Simple DYLD_INTERPOSE solution that just intercepts without calling original

#include "ggml_hook.hpp"
#include "ggml-impl.h"
#include "ggml-backend.h"
#include <stdio.h>
#include <unistd.h>

// Forward declare scheduler type
typedef struct ggml_backend_sched * ggml_backend_sched_t;

// DYLD_INTERPOSE macro for guaranteed symbol replacement
#define DYLD_INTERPOSE(_repl,_orig) \
  __attribute__((used)) static struct{ const void* repl; const void* orig; } _interpose_##_orig \
  __attribute__((section("__DATA,__interpose"))) = { (const void*)(unsigned long)&_repl, (const void*)(unsigned long)&_orig };

extern "C" {
    // Forward declarations
    enum ggml_status ggml_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cgraph);
    enum ggml_status ggml_backend_graph_compute_async(ggml_backend_t backend, struct ggml_cgraph* cgraph);
    void ggml_graph_compute(struct ggml_context* ctx, struct ggml_cgraph* cgraph);
    enum ggml_status ggml_graph_compute_with_ctx(struct ggml_context* ctx, struct ggml_cgraph* cgraph, int n_threads);
    
    // Scheduler functions - the modern hot path (weak to avoid linking errors)
    enum ggml_status ggml_backend_sched_graph_compute(ggml_backend_sched_t sched, struct ggml_cgraph* cgraph) __attribute__((weak));
    enum ggml_status ggml_backend_sched_graph_compute_async(ggml_backend_sched_t sched, struct ggml_cgraph* cgraph) __attribute__((weak));
    
    // Dummy implementations of functions that exist in llama.cpp
    enum ggml_status viz_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cgraph) {
        auto& hook = ggml_viz::GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] DYLD_INTERPOSE: Intercepted ggml_backend_graph_compute, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, backend);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], backend);
                }
            }
            
            // Call each node's end hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], backend);
                }
            }
            
            hook.on_graph_compute_end(cgraph, backend);
        }
        
        return GGML_STATUS_SUCCESS;
    }
    
    enum ggml_status viz_backend_graph_compute_async(ggml_backend_t backend, struct ggml_cgraph* cgraph) {
        auto& hook = ggml_viz::GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] DYLD_INTERPOSE: Intercepted ggml_backend_graph_compute_async, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, backend);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], backend);
                }
            }
            
            // Call each node's end hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], backend);
                }
            }
            
            hook.on_graph_compute_end(cgraph, backend);
        }
        
        return GGML_STATUS_SUCCESS;
    }
    
    void viz_graph_compute(struct ggml_context* ctx, struct ggml_cgraph* cgraph) {
        auto& hook = ggml_viz::GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] DYLD_INTERPOSE: Intercepted ggml_graph_compute, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, nullptr);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], nullptr);
                }
            }
            
            // Call each node's end hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], nullptr);
                }
            }
            
            hook.on_graph_compute_end(cgraph, nullptr);
        }
    }
    
    enum ggml_status viz_graph_compute_with_ctx(struct ggml_context* ctx, struct ggml_cgraph* cgraph, int n_threads) {
        auto& hook = ggml_viz::GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] DYLD_INTERPOSE: Intercepted ggml_graph_compute_with_ctx, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, nullptr);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], nullptr);
                }
            }
            
            // Call each node's end hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], nullptr);
                }
            }
            
            hook.on_graph_compute_end(cgraph, nullptr);
        }
        
        return GGML_STATUS_SUCCESS;
    }
    
    // TODO: Add scheduler interposers later
    // Scheduler interposers would go here for ggml_backend_sched_graph_compute
}

// DYLD_INTERPOSE directives - these guarantee symbol replacement
DYLD_INTERPOSE(viz_backend_graph_compute, ggml_backend_graph_compute)
DYLD_INTERPOSE(viz_graph_compute, ggml_graph_compute)
DYLD_INTERPOSE(viz_graph_compute_with_ctx, ggml_graph_compute_with_ctx)

// Scheduler interposers - the modern hot path that catches all backends
// Only add if the symbols exist in the target
// DYLD_INTERPOSE(viz_sched_graph_compute, ggml_backend_sched_graph_compute)
// DYLD_INTERPOSE(viz_sched_graph_compute_async, ggml_backend_sched_graph_compute_async)