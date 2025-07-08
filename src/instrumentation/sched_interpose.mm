//  sched_interpose.mm  – catches the modern ggml scheduler path
//  (compile as Objective-C++ to access C++ headers)

#import "ggml_hook.hpp"
#include "ggml-backend.h"
#include "ggml-impl.h"          // for struct ggml_cgraph

// Forward-declare the scheduler type used in headers
typedef struct ggml_backend_sched * ggml_backend_sched_t;

// Replacement functions - capture events without calling original
extern "C" {
__attribute__((visibility("default")))
enum ggml_status viz_sched_graph_compute(ggml_backend_sched_t sched, struct ggml_cgraph* cg) {
    fprintf(stderr, "[GGML_VIZ] *** SCHEDULER INTERPOSED: viz_sched_graph_compute called! ***\n");
    
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT"))
        hook.start();

    if (hook.is_active()) {
        fprintf(stderr, "[GGML_VIZ] sched_graph_compute  n_nodes=%d\n", cg->n_nodes);
        hook.on_graph_compute_begin(cg, reinterpret_cast<const ggml_backend*>(sched));
        hook.on_graph_compute_end(cg, reinterpret_cast<const ggml_backend*>(sched));
    }

    return GGML_STATUS_SUCCESS;
}

__attribute__((visibility("default")))
enum ggml_status viz_sched_graph_compute_async(ggml_backend_sched_t sched, struct ggml_cgraph* cg) {
    fprintf(stderr, "[GGML_VIZ] *** SCHEDULER INTERPOSED: viz_sched_graph_compute_async called! ***\n");
    
    auto& hook = ggml_viz::GGMLHook::instance();

    if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT"))
        hook.start();

    if (hook.is_active()) {
        fprintf(stderr, "[GGML_VIZ] sched_graph_compute_async  n_nodes=%d\n", cg->n_nodes);
        hook.on_graph_compute_begin(cg, reinterpret_cast<const ggml_backend*>(sched));
        hook.on_graph_compute_end(cg, reinterpret_cast<const ggml_backend*>(sched));
    }

    return GGML_STATUS_SUCCESS;
}
}

// --- DYLD interpose glue --------------------------------------------------
#define DYLD_INTERPOSE(_repl,_orig) \
__attribute__((used)) static struct{ const void* repl; const void* orig; } \
_interpose_##_orig  __attribute__((section("__DATA,__interpose"))) = { \
    (const void*)(unsigned long)&_repl, (const void*)(unsigned long)&_orig };

DYLD_INTERPOSE(viz_sched_graph_compute,       ggml_backend_sched_graph_compute)
DYLD_INTERPOSE(viz_sched_graph_compute_async, ggml_backend_sched_graph_compute_async)