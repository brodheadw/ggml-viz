// src/instrumentation/dyld_interpose.cpp
// DYLD_INTERPOSE solution for robust symbol interposition on macOS

#include "ggml_hook.hpp"
#include "ggml-impl.h"
#include "ggml-backend.h"
#include <stdio.h>
#include <unistd.h>

// DYLD_INTERPOSE macro for guaranteed symbol replacement
#define DYLD_INTERPOSE(_repl,_orig) \
  __attribute__((used)) static struct{ const void* repl; const void* orig; } _interpose_##_orig \
  __attribute__((section("__DATA,__interpose"))) = { (const void*)(unsigned long)&_repl, (const void*)(unsigned long)&_orig };

extern "C" {
    // Forward declarations of original functions - we'll use dlsym instead
    typedef enum ggml_status (*ggml_backend_graph_compute_func)(ggml_backend_t backend, struct ggml_cgraph* cgraph);
    typedef enum ggml_status (*ggml_backend_graph_compute_async_func)(ggml_backend_t backend, struct ggml_cgraph* cgraph);
    typedef enum ggml_status (*ggml_backend_metal_graph_compute_func)(ggml_backend_t backend, struct ggml_cgraph* cgraph);
    typedef void (*ggml_graph_compute_func)(struct ggml_context* ctx, struct ggml_cgraph* cgraph);
    typedef enum ggml_status (*ggml_graph_compute_with_ctx_func)(struct ggml_context* ctx, struct ggml_cgraph* cgraph, int n_threads);
    
    static ggml_backend_graph_compute_func orig_backend_graph_compute = nullptr;
    static ggml_backend_graph_compute_async_func orig_backend_graph_compute_async = nullptr;
    static ggml_backend_metal_graph_compute_func orig_backend_metal_graph_compute = nullptr;
    static ggml_graph_compute_func orig_graph_compute = nullptr;
    static ggml_graph_compute_with_ctx_func orig_graph_compute_with_ctx = nullptr;
    
    // Initialize function pointers
    static void init_original_functions() {
        static bool initialized = false;
        if (initialized) return;
        
        void* handle = dlopen(NULL, RTLD_LAZY);
        if (handle) {
            orig_backend_graph_compute = (ggml_backend_graph_compute_func)dlsym(handle, "ggml_backend_graph_compute");
            orig_backend_graph_compute_async = (ggml_backend_graph_compute_async_func)dlsym(handle, "ggml_backend_graph_compute_async");
            orig_backend_metal_graph_compute = (ggml_backend_metal_graph_compute_func)dlsym(handle, "ggml_backend_metal_graph_compute");
            orig_graph_compute = reinterpret_cast<ggml_graph_compute_func>(dlsym(handle, "ggml_graph_compute"));
            orig_graph_compute_with_ctx = (ggml_graph_compute_with_ctx_func)dlsym(handle, "ggml_graph_compute_with_ctx");
            dlclose(handle);
        }
        initialized = true;
    }
    
    // Interposer functions
    enum ggml_status viz_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cgraph) {
        init_original_functions();
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
        }
        
        // Call the original function if it exists
        enum ggml_status result = GGML_STATUS_SUCCESS;
        if (orig_backend_graph_compute) {
            result = orig_backend_graph_compute(backend, cgraph);
        }
        
        if (hook.is_active()) {
            // Call each node's end hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], backend);
                }
            }
            
            hook.on_graph_compute_end(cgraph, backend);
        }
        
        return result;
    }
    
    enum ggml_status viz_backend_graph_compute_async(ggml_backend_t backend, struct ggml_cgraph* cgraph) {
        init_original_functions();
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
        }
        
        // Call the original function if it exists
        enum ggml_status result = GGML_STATUS_SUCCESS;
        if (orig_backend_graph_compute_async) {
            result = orig_backend_graph_compute_async(backend, cgraph);
        }
        
        if (hook.is_active()) {
            // Call each node's end hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], backend);
                }
            }
            
            hook.on_graph_compute_end(cgraph, backend);
        }
        
        return result;
    }
    
    enum ggml_status viz_backend_metal_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cgraph) {
        init_original_functions();
        auto& hook = ggml_viz::GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] DYLD_INTERPOSE: Intercepted ggml_backend_metal_graph_compute, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, backend);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], backend);
                }
            }
        }
        
        // Call the original function if it exists
        enum ggml_status result = GGML_STATUS_SUCCESS;
        if (orig_backend_metal_graph_compute) {
            result = orig_backend_metal_graph_compute(backend, cgraph);
        }
        
        if (hook.is_active()) {
            // Call each node's end hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], backend);
                }
            }
            
            hook.on_graph_compute_end(cgraph, backend);
        }
        
        return result;
    }
    
    void viz_graph_compute(struct ggml_context* ctx, struct ggml_cgraph* cgraph) {
        init_original_functions();
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
        }
        
        // Call the original function if it exists
        if (orig_graph_compute) {
            orig_graph_compute(ctx, cgraph);
        }
        
        if (hook.is_active()) {
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
        init_original_functions();
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
        }
        
        // Call the original function if it exists
        enum ggml_status result = GGML_STATUS_SUCCESS;
        if (orig_graph_compute_with_ctx) {
            result = orig_graph_compute_with_ctx(ctx, cgraph, n_threads);
        }
        
        if (hook.is_active()) {
            // Call each node's end hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_end(cgraph->nodes[i], nullptr);
                }
            }
            
            hook.on_graph_compute_end(cgraph, nullptr);
        }
        
        return result;
    }
}

// DYLD_INTERPOSE directives - these guarantee symbol replacement
DYLD_INTERPOSE(viz_backend_graph_compute, ggml_backend_graph_compute)
DYLD_INTERPOSE(viz_graph_compute, ggml_graph_compute)
DYLD_INTERPOSE(viz_graph_compute_with_ctx, ggml_graph_compute_with_ctx)

// Only interpose if the symbols exist (conditional compilation)
#ifdef GGML_BACKEND_GRAPH_COMPUTE_ASYNC_EXISTS
DYLD_INTERPOSE(viz_backend_graph_compute_async, ggml_backend_graph_compute_async)
#endif

#ifdef GGML_BACKEND_METAL_GRAPH_COMPUTE_EXISTS
DYLD_INTERPOSE(viz_backend_metal_graph_compute, ggml_backend_metal_graph_compute)
#endif