//  linux_interpose.cpp  – Linux LD_PRELOAD implementation for GGML scheduler interception
//  Similar to macOS sched_interpose.mm but using Linux symbol interposition

#include "ggml_hook.hpp"
#include "ggml-backend.h"
#include "ggml-impl.h"          // for struct ggml_cgraph
#include "ggml.h"               // for ggml_context and ggml_status
#include "ggml-cpu.h"           // for ggml_graph_compute declarations
#include <dlfcn.h>              // for dlsym
#include <cstdlib>              // for getenv

// Forward-declare the scheduler type used in headers
typedef struct ggml_backend_sched * ggml_backend_sched_t;

// Linux symbol interposition works by defining functions with the same name
// When loaded with LD_PRELOAD, these override the originals
extern "C" {

// Modern scheduler functions - the primary interception points
enum ggml_status ggml_backend_sched_graph_compute(ggml_backend_sched_t sched, struct ggml_cgraph* cg) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, reinterpret_cast<const ggml_backend*>(sched));
        
        // Capture individual operation events
        if (cg && cg->nodes) {
            for (int i = 0; i < cg->n_nodes; i++) {
                if (cg->nodes[i]) {
                    hook.on_op_compute_begin(cg->nodes[i], reinterpret_cast<const ggml_backend*>(sched));
                }
            }
        }
    }

    // Call the original function
    using fn_type = enum ggml_status (*)(ggml_backend_sched_t, struct ggml_cgraph*);
    static fn_type real_fn = (fn_type)dlsym(RTLD_NEXT, "ggml_backend_sched_graph_compute");
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        rc = real_fn(sched, cg);  // delegate to actual compute
    }

    if (hook.is_active()) {
        // Capture individual operation end events
        if (cg && cg->nodes) {
            for (int i = 0; i < cg->n_nodes; i++) {
                if (cg->nodes[i]) {
                    hook.on_op_compute_end(cg->nodes[i], reinterpret_cast<const ggml_backend*>(sched));
                }
            }
        }
        
        hook.on_graph_compute_end(cg, reinterpret_cast<const ggml_backend*>(sched));
    }

    return rc;
}

enum ggml_status ggml_backend_sched_graph_compute_async(ggml_backend_sched_t sched, struct ggml_cgraph* cg) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, reinterpret_cast<const ggml_backend*>(sched));
        
        // Capture individual operation events
        if (cg && cg->nodes) {
            for (int i = 0; i < cg->n_nodes; i++) {
                if (cg->nodes[i]) {
                    hook.on_op_compute_begin(cg->nodes[i], reinterpret_cast<const ggml_backend*>(sched));
                }
            }
        }
    }

    // Call the original function
    using fn_type = enum ggml_status (*)(ggml_backend_sched_t, struct ggml_cgraph*);
    static fn_type real_fn = (fn_type)dlsym(RTLD_NEXT, "ggml_backend_sched_graph_compute_async");
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        rc = real_fn(sched, cg);  // delegate to actual compute
    }

    if (hook.is_active()) {
        // Capture individual operation end events
        if (cg && cg->nodes) {
            for (int i = 0; i < cg->n_nodes; i++) {
                if (cg->nodes[i]) {
                    hook.on_op_compute_end(cg->nodes[i], reinterpret_cast<const ggml_backend*>(sched));
                }
            }
        }
        
        hook.on_graph_compute_end(cg, reinterpret_cast<const ggml_backend*>(sched));
    }

    return rc;
}

// Backend graph compute functions
enum ggml_status ggml_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cg) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, reinterpret_cast<const ggml_backend*>(backend));
        
        // Capture individual operation events
        if (cg && cg->nodes) {
            for (int i = 0; i < cg->n_nodes; i++) {
                if (cg->nodes[i]) {
                    hook.on_op_compute_begin(cg->nodes[i], reinterpret_cast<const ggml_backend*>(backend));
                }
            }
        }
    }

    // Call the original function
    using fn_type = enum ggml_status (*)(ggml_backend_t, struct ggml_cgraph*);
    static fn_type real_fn = (fn_type)dlsym(RTLD_NEXT, "ggml_backend_graph_compute");
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        rc = real_fn(backend, cg);  // delegate to actual compute
    }

    if (hook.is_active()) {
        // Capture individual operation end events
        if (cg && cg->nodes) {
            for (int i = 0; i < cg->n_nodes; i++) {
                if (cg->nodes[i]) {
                    hook.on_op_compute_end(cg->nodes[i], reinterpret_cast<const ggml_backend*>(backend));
                }
            }
        }
        
        hook.on_graph_compute_end(cg, reinterpret_cast<const ggml_backend*>(backend));
    }

    return rc;
}

enum ggml_status ggml_backend_graph_compute_async(ggml_backend_t backend, struct ggml_cgraph* cg) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, reinterpret_cast<const ggml_backend*>(backend));
        
        // Capture individual operation events
        if (cg && cg->nodes) {
            for (int i = 0; i < cg->n_nodes; i++) {
                if (cg->nodes[i]) {
                    hook.on_op_compute_begin(cg->nodes[i], reinterpret_cast<const ggml_backend*>(backend));
                }
            }
        }
    }

    // Call the original function
    using fn_type = enum ggml_status (*)(ggml_backend_t, struct ggml_cgraph*);
    static fn_type real_fn = (fn_type)dlsym(RTLD_NEXT, "ggml_backend_graph_compute_async");
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        rc = real_fn(backend, cg);  // delegate to actual compute
    }

    if (hook.is_active()) {
        // Capture individual operation end events
        if (cg && cg->nodes) {
            for (int i = 0; i < cg->n_nodes; i++) {
                if (cg->nodes[i]) {
                    hook.on_op_compute_end(cg->nodes[i], reinterpret_cast<const ggml_backend*>(backend));
                }
            }
        }
        
        hook.on_graph_compute_end(cg, reinterpret_cast<const ggml_backend*>(backend));
    }

    return rc;
}

// Legacy graph compute functions for CPU-only paths
enum ggml_status ggml_graph_compute(struct ggml_cgraph* cg, struct ggml_cplan* cplan) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, nullptr);
    }

    // Call the original function
    using fn_type = enum ggml_status (*)(struct ggml_cgraph*, struct ggml_cplan*);
    static fn_type real_fn = (fn_type)dlsym(RTLD_NEXT, "ggml_graph_compute");
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        rc = real_fn(cg, cplan);  // delegate to actual compute
    }

    if (hook.is_active()) {
        hook.on_graph_compute_end(cg, nullptr);
    }

    return rc;
}

enum ggml_status ggml_graph_compute_with_ctx(struct ggml_context* ctx, struct ggml_cgraph* cg, int n_threads) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, nullptr);
    }

    // Call the original function
    using fn_type = enum ggml_status (*)(struct ggml_context*, struct ggml_cgraph*, int);
    static fn_type real_fn = (fn_type)dlsym(RTLD_NEXT, "ggml_graph_compute_with_ctx");
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        rc = real_fn(ctx, cg, n_threads);  // delegate to actual compute
    }

    if (hook.is_active()) {
        hook.on_graph_compute_end(cg, nullptr);
    }

    return rc;
}

} // extern "C"