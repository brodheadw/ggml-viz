//  sched_interpose.mm  – catches the modern ggml scheduler path
//  (compile as Objective-C++ to access C++ headers)

#import "ggml_hook.hpp"
#include "ggml-backend.h"
#include "ggml-alloc.h"
#include "ggml-impl.h"          // for struct ggml_cgraph
#include "ggml.h"               // for ggml_context and ggml_status
#include "ggml-cpu.h"           // for ggml_graph_compute declarations
#include <dlfcn.h>              // for dlsym

// Forward-declare the scheduler type used in headers
typedef struct ggml_backend_sched * ggml_backend_sched_t;

namespace ggml_viz {
enum ggml_status sched_compute(ggml_backend_sched_t sched, struct ggml_cgraph* graph,
                               enum ggml_status (*real_fn)(ggml_backend_sched_t, struct ggml_cgraph*));
void sched_set_eval_callback(ggml_backend_sched_t sched, ggml_backend_sched_eval_callback cb, void* user_data);
}

// Replacement functions - capture events AND call original functions.
// Calls by name reach the originals: dyld doesn't apply interposing to the interposing image,
// and dlsym(RTLD_NEXT) returns null for a DYLD_INSERT_LIBRARIES library.
extern "C" {
__attribute__((visibility("default")))
enum ggml_status viz_sched_graph_compute(ggml_backend_sched_t sched, struct ggml_cgraph* cg) {
    using fn_type = enum ggml_status (*)(ggml_backend_sched_t, struct ggml_cgraph*);
    static fn_type real_fn = &ggml_backend_sched_graph_compute;
    return ggml_viz::sched_compute(sched, cg, real_fn);
}

__attribute__((visibility("default")))
enum ggml_status viz_sched_graph_compute_async(ggml_backend_sched_t sched, struct ggml_cgraph* cg) {
    using fn_type = enum ggml_status (*)(ggml_backend_sched_t, struct ggml_cgraph*);
    static fn_type real_fn = &ggml_backend_sched_graph_compute_async;
    return ggml_viz::sched_compute(sched, cg, real_fn);
}

__attribute__((visibility("default")))
void viz_sched_set_eval_callback(ggml_backend_sched_t sched, ggml_backend_sched_eval_callback cb, void* user_data) {
    ggml_viz::sched_set_eval_callback(sched, cb, user_data);
}

__attribute__((visibility("default")))
enum ggml_status viz_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cg) {
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }

    if (hook.is_active()) {
        hook.on_graph_compute_begin(cg, reinterpret_cast<const ggml_backend*>(backend));
        
    }

    // Call the original function if it exists
    using fn_type = enum ggml_status (*)(ggml_backend_t, struct ggml_cgraph*);
    static fn_type real_fn = &ggml_backend_graph_compute;
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        ggml_viz::NestedComputeScope scope;
        rc = real_fn(backend, cg);  // delegate to actual compute
    }

    if (hook.is_active()) {
        
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
        
    }

    // Call the original function if it exists
    using fn_type = enum ggml_status (*)(ggml_backend_t, struct ggml_cgraph*);
    static fn_type real_fn = &ggml_backend_graph_compute_async;
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        ggml_viz::NestedComputeScope scope;
        rc = real_fn(backend, cg);  // delegate to actual compute
    }

    if (hook.is_active()) {
        
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
    static fn_type real_fn = &ggml_graph_compute;
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        ggml_viz::NestedComputeScope scope;
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
    static fn_type real_fn = &ggml_graph_compute_with_ctx;
    
    enum ggml_status rc = GGML_STATUS_SUCCESS;
    if (real_fn) {
        ggml_viz::NestedComputeScope scope;
        rc = real_fn(ctx, cg, n_threads);  // delegate to actual compute
    }

    if (hook.is_active()) {
        hook.on_graph_compute_end(cg, nullptr);
    }

    return rc;
}
}

// Allocation: only calls from other images are interposed (ggml's internal tallocr/gallocr calls are not),
// so record the tensors a context allocation placed in its new buffer.
static void record_ctx_allocs(struct ggml_context* ctx, ggml_backend_buffer_t buf) {
    auto& hook = ggml_viz::GGMLHook::instance();
    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }
    if (!buf || !hook.is_active()) return;
    for (struct ggml_tensor* t = ggml_get_first_tensor(ctx); t; t = ggml_get_next_tensor(ctx, t)) {
        if (t->buffer == buf && !t->view_src) {
            hook.on_tensor_alloc(t, ggml_nbytes(t), nullptr);
        }
    }
}

extern "C" {
__attribute__((visibility("default")))
ggml_backend_buffer_t viz_alloc_ctx_tensors(struct ggml_context* ctx, ggml_backend_t backend) {
    ggml_backend_buffer_t buf = ggml_backend_alloc_ctx_tensors(ctx, backend);
    record_ctx_allocs(ctx, buf);
    return buf;
}

// Scheduler allocations happen inside ggml (gallocr), out of reach; record the graph's tensors as Linux does
__attribute__((visibility("default")))
bool viz_sched_alloc_graph(ggml_backend_sched_t sched, struct ggml_cgraph* graph) {
    bool ok = ggml_backend_sched_alloc_graph(sched, graph);
    auto& hook = ggml_viz::GGMLHook::instance();
    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
        hook.start();
    }
    if (ok && graph && hook.is_active()) {
        for (int i = 0; i < graph->n_nodes; i++) {
            if (!graph->nodes[i]->view_src) hook.on_tensor_alloc(graph->nodes[i], ggml_nbytes(graph->nodes[i]), nullptr);
        }
        for (int i = 0; i < graph->n_leafs; i++) {
            if (!graph->leafs[i]->view_src) hook.on_tensor_alloc(graph->leafs[i], ggml_nbytes(graph->leafs[i]), nullptr);
        }
    }
    return ok;
}

__attribute__((visibility("default")))
ggml_backend_buffer_t viz_alloc_ctx_tensors_from_buft(struct ggml_context* ctx, ggml_backend_buffer_type_t buft) {
    ggml_backend_buffer_t buf = ggml_backend_alloc_ctx_tensors_from_buft(ctx, buft);
    record_ctx_allocs(ctx, buf);
    return buf;
}
}

// --- DYLD interpose glue --------------------------------------------------
#define DYLD_INTERPOSE(_repl,_orig) \
__attribute__((used)) static struct{ const void* repl; const void* orig; } \
_interpose_##_orig  __attribute__((section("__DATA,__interpose"))) = { \
    (const void*)(unsigned long)&_repl, (const void*)(unsigned long)&_orig };

DYLD_INTERPOSE(viz_sched_graph_compute,       ggml_backend_sched_graph_compute)
DYLD_INTERPOSE(viz_sched_graph_compute_async, ggml_backend_sched_graph_compute_async)
DYLD_INTERPOSE(viz_sched_set_eval_callback,   ggml_backend_sched_set_eval_callback)
DYLD_INTERPOSE(viz_backend_graph_compute,     ggml_backend_graph_compute)
DYLD_INTERPOSE(viz_backend_graph_compute_async, ggml_backend_graph_compute_async)
DYLD_INTERPOSE(viz_graph_compute,             ggml_graph_compute)
DYLD_INTERPOSE(viz_graph_compute_with_ctx,    ggml_graph_compute_with_ctx)
DYLD_INTERPOSE(viz_alloc_ctx_tensors,          ggml_backend_alloc_ctx_tensors)
DYLD_INTERPOSE(viz_alloc_ctx_tensors_from_buft, ggml_backend_alloc_ctx_tensors_from_buft)
DYLD_INTERPOSE(viz_sched_alloc_graph,           ggml_backend_sched_alloc_graph)
