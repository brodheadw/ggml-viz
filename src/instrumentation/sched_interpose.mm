//  sched_interpose.mm  – catches the modern ggml scheduler path
//  (compile as Objective-C++ to access C++ headers)

#import "ggml_hook.hpp"
#include "ggml-backend.h"
#include "ggml-impl.h"          // for struct ggml_cgraph
#include "ggml.h"               // for ggml_context and ggml_status
#include "ggml-cpu.h"           // for ggml_graph_compute declarations
#include <dlfcn.h>              // for dlsym

// Forward-declare the scheduler type used in headers
typedef struct ggml_backend_sched * ggml_backend_sched_t;

// Replacement functions - capture events AND call original functions
extern "C" {
__attribute__((visibility("default")))
enum ggml_status viz_sched_graph_compute(ggml_backend_sched_t sched, struct ggml_cgraph* cg) {
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

    // Call the original function if it exists
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

__attribute__((visibility("default")))
enum ggml_status viz_sched_graph_compute_async(ggml_backend_sched_t sched, struct ggml_cgraph* cg) {
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

    // Call the original function if it exists
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

// Additional backend graph compute functions
__attribute__((visibility("default")))
enum ggml_status viz_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cg) {
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

    // Call the original function if it exists
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

__attribute__((visibility("default")))
enum ggml_status viz_backend_graph_compute_async(ggml_backend_t backend, struct ggml_cgraph* cg) {
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

    // Call the original function if it exists
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
__attribute__((visibility("default")))
enum ggml_status viz_graph_compute(struct ggml_cgraph* cg, struct ggml_cplan* cplan) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, nullptr);
    }

    // Call the original function if it exists
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

__attribute__((visibility("default")))
enum ggml_status viz_graph_compute_with_ctx(struct ggml_context* ctx, struct ggml_cgraph* cg, int n_threads) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, nullptr);
    }

    // Call the original function if it exists
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

// Backend memory allocation tracking
__attribute__((visibility("default")))
ggml_backend_buffer_t viz_backend_buft_alloc_buffer(ggml_backend_buffer_type_t buft, size_t size) {
    auto& hook = ggml_viz::GGMLHook::instance();
    
    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }
    
    // Call original function
    static ggml_backend_buffer_t (*original_alloc)(ggml_backend_buffer_type_t, size_t) = nullptr;
    if (!original_alloc) {
        original_alloc = (ggml_backend_buffer_t (*)(ggml_backend_buffer_type_t, size_t))dlsym(RTLD_NEXT, "ggml_backend_buft_alloc_buffer");
    }
    
    ggml_backend_buffer_t buffer = original_alloc ? original_alloc(buft, size) : nullptr;
    
    // Track allocation
    if (buffer && hook.is_active()) {
        hook.on_backend_buffer_alloc(buffer, size);
    }
    
    return buffer;
}

__attribute__((visibility("default")))
void viz_backend_buffer_free(ggml_backend_buffer_t buffer) {
    auto& hook = ggml_viz::GGMLHook::instance();
    
    // Track deallocation before freeing
    if (buffer && hook.is_active()) {
        hook.on_backend_buffer_free(buffer);
    }
    
    // Call original function
    static void (*original_free)(ggml_backend_buffer_t) = nullptr;
    if (!original_free) {
        original_free = (void (*)(ggml_backend_buffer_t))dlsym(RTLD_NEXT, "ggml_backend_buffer_free");
    }
    
    if (original_free) {
        original_free(buffer);
    }
}
}

// --- DYLD interpose glue --------------------------------------------------
#define DYLD_INTERPOSE(_repl,_orig) \
__attribute__((used)) static struct{ const void* repl; const void* orig; } \
_interpose_##_orig  __attribute__((section("__DATA,__interpose"))) = { \
    (const void*)(unsigned long)&_repl, (const void*)(unsigned long)&_orig };

DYLD_INTERPOSE(viz_sched_graph_compute,       ggml_backend_sched_graph_compute)
DYLD_INTERPOSE(viz_sched_graph_compute_async, ggml_backend_sched_graph_compute_async)
DYLD_INTERPOSE(viz_backend_graph_compute,     ggml_backend_graph_compute)
DYLD_INTERPOSE(viz_backend_graph_compute_async, ggml_backend_graph_compute_async)
DYLD_INTERPOSE(viz_graph_compute,             ggml_graph_compute)
DYLD_INTERPOSE(viz_graph_compute_with_ctx,    ggml_graph_compute_with_ctx)

// Memory allocation interposition
DYLD_INTERPOSE(viz_backend_buft_alloc_buffer, ggml_backend_buft_alloc_buffer)
DYLD_INTERPOSE(viz_backend_buffer_free,       ggml_backend_buffer_free)