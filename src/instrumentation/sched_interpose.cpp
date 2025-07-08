// src/instrumentation/sched_interpose.cpp
// Scheduler-specific DYLD_INTERPOSE implementation

#include "ggml_hook.hpp"
#include "ggml-impl.h"
#include "ggml-backend.h"
#include <stdio.h>

// DYLD_INTERPOSE macro for guaranteed symbol replacement
#define DYLD_INTERPOSE(_repl,_orig) \
  __attribute__((used)) static struct{ const void* repl; const void* orig; } _interpose_##_orig \
  __attribute__((section("__DATA,__interpose"))) = { (const void*)(unsigned long)&_repl, (const void*)(unsigned long)&_orig };

extern "C" {
    // Scheduler interposer functions
    enum ggml_status viz_sched_graph_compute(ggml_backend_sched_t sched, struct ggml_cgraph* cgraph) {
        auto& hook = ggml_viz::GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] DYLD_INTERPOSE: Intercepted ggml_backend_sched_graph_compute, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, reinterpret_cast<const ggml_backend*>(sched));
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], reinterpret_cast<const ggml_backend*>(sched));
                }
            }
            
            // Call each node's end hook immediately (no real backend call)
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], reinterpret_cast<const ggml_backend*>(sched));
                }
            }
            
            hook.on_graph_compute_end(cgraph, reinterpret_cast<const ggml_backend*>(sched));
        }
        
        return GGML_STATUS_SUCCESS;
    }
    
    enum ggml_status viz_sched_graph_compute_async(ggml_backend_sched_t sched, struct ggml_cgraph* cgraph) {
        auto& hook = ggml_viz::GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] DYLD_INTERPOSE: Intercepted ggml_backend_sched_graph_compute_async, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, reinterpret_cast<const ggml_backend*>(sched));
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], reinterpret_cast<const ggml_backend*>(sched));
                }
            }
            
            // Call each node's end hook immediately (no real backend call)
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], reinterpret_cast<const ggml_backend*>(sched));
                }
            }
            
            hook.on_graph_compute_end(cgraph, reinterpret_cast<const ggml_backend*>(sched));
        }
        
        return GGML_STATUS_SUCCESS;
    }
}

// The key interposers that will catch all backends
DYLD_INTERPOSE(viz_sched_graph_compute, ggml_backend_sched_graph_compute)
DYLD_INTERPOSE(viz_sched_graph_compute_async, ggml_backend_sched_graph_compute_async)