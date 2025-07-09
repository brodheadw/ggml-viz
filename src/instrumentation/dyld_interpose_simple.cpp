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
    
    // Functions implemented in sched_interpose.mm - removed from here to avoid duplicates
    
    // TODO: Add scheduler interposers later
    // Scheduler interposers would go here for ggml_backend_sched_graph_compute
}

// DYLD_INTERPOSE directives moved to sched_interpose.mm to avoid duplicates
// This file is kept for potential future use or reference