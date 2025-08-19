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

// ===================== ggml-viz Metal swizzles =====================
#import <Metal/Metal.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <unordered_map>
#import <unordered_set>
#import <algorithm>
#import <mutex>

namespace ggml_viz { class GGMLHook; } // fwd
extern ggml_viz::GGMLHook& ggml_viz_get_hook(); // if you prefer, replace with GGMLHook::instance()

static const void *kVizBufSizeKey = &kVizBufSizeKey;

static std::mutex g_map_mtx;

// Per-class original IMP maps
static std::unordered_map<Class, IMP> g_orig_dev_len_opts;
static std::unordered_map<Class, IMP> g_orig_dev_bytes_len_opts;
static std::unordered_map<Class, IMP> g_orig_heap_len_opts;
static std::unordered_map<Class, IMP> g_orig_dealloc_by_cls;
static std::unordered_set<Class>      g_dealloc_swizzled;

// -------- helpers
static void swizzle_method(Class cls, SEL sel, IMP repl, std::unordered_map<Class, IMP> &store) {
    if (!cls || !sel) return;
    Method m = class_getInstanceMethod(cls, sel);
    if (!m) return;
    std::lock_guard<std::mutex> lock(g_map_mtx);
    if (store.find(cls) != store.end()) return; // already
    IMP old = method_getImplementation(m);
    method_setImplementation(m, repl);
    store.emplace(cls, old);
}

static void swizzle_dealloc_for_buffer_class_if_needed(id<MTLBuffer> buf);

// -------- replacements
static id<MTLBuffer> repl_dev_newBufferWithLength_options(id self, SEL _cmd, NSUInteger length, MTLResourceOptions opts) {
    Class cls = object_getClass((id)self);
    IMP old;
    {
        std::lock_guard<std::mutex> lock(g_map_mtx);
        old = g_orig_dev_len_opts[cls];
    }
    auto orig = (id<MTLBuffer>(*)(id,SEL,NSUInteger,MTLResourceOptions))old;
    id<MTLBuffer> b = orig ? orig(self,_cmd,length,opts) : nil;
    if (b) {
        objc_setAssociatedObject((id)b, kVizBufSizeKey, @(length), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        swizzle_dealloc_for_buffer_class_if_needed(b);
        ggml_viz_get_hook().on_backend_buffer_alloc((void*)(__bridge void*)b, (size_t)length);
    }
    return b;
}

static id<MTLBuffer> repl_dev_newBufferWithBytes_length_options(id self, SEL _cmd, const void *bytes, NSUInteger length, MTLResourceOptions opts) {
    Class cls = object_getClass((id)self);
    IMP old;
    {
        std::lock_guard<std::mutex> lock(g_map_mtx);
        old = g_orig_dev_bytes_len_opts[cls];
    }
    auto orig = (id<MTLBuffer>(*)(id,SEL,const void*,NSUInteger,MTLResourceOptions))old;
    id<MTLBuffer> b = orig ? orig(self,_cmd,bytes,length,opts) : nil;
    if (b) {
        objc_setAssociatedObject((id)b, kVizBufSizeKey, @(length), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        swizzle_dealloc_for_buffer_class_if_needed(b);
        ggml_viz_get_hook().on_backend_buffer_alloc((void*)(__bridge void*)b, (size_t)length);
    }
    return b;
}

static id<MTLBuffer> repl_heap_newBufferWithLength_options(id self, SEL _cmd, NSUInteger length, MTLResourceOptions opts) {
    Class cls = object_getClass((id)self);
    IMP old;
    {
        std::lock_guard<std::mutex> lock(g_map_mtx);
        old = g_orig_heap_len_opts[cls];
    }
    auto orig = (id<MTLBuffer>(*)(id,SEL,NSUInteger,MTLResourceOptions))old;
    id<MTLBuffer> b = orig ? orig(self,_cmd,length,opts) : nil;
    if (b) {
        objc_setAssociatedObject((id)b, kVizBufSizeKey, @(length), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        swizzle_dealloc_for_buffer_class_if_needed(b);
        ggml_viz_get_hook().on_backend_buffer_alloc((void*)(__bridge void*)b, (size_t)length);
    }
    return b;
}

// dealloc swizzle (per concrete MTLBuffer subclass)
static void repl_buffer_dealloc(id self, SEL _cmd) {
    // capture size before it disappears
    NSNumber *n = (NSNumber *)objc_getAssociatedObject(self, kVizBufSizeKey);
    if (n) {
        ggml_viz_get_hook().on_backend_buffer_free((void*)self);
        objc_setAssociatedObject(self, kVizBufSizeKey, nil, OBJC_ASSOCIATION_ASSIGN);
    }
    // call original dealloc for this concrete class
    Class cls = object_getClass(self);
    IMP old;
    {
        std::lock_guard<std::mutex> lock(g_map_mtx);
        old = g_orig_dealloc_by_cls[cls];
    }
    auto orig = (void(*)(id,SEL))old;
    if (orig) orig(self, _cmd);
    else {
        // Fallback: call super dealloc if needed (rarely necessary)
        struct objc_super sup = { self, class_getSuperclass(cls) };
        ((void(*)(struct objc_super*, SEL))objc_msgSendSuper)(&sup, _cmd);
    }
}

static void swizzle_dealloc_for_buffer_class_if_needed(id<MTLBuffer> buf) {
    Class cls = object_getClass((id)buf);
    std::lock_guard<std::mutex> lock(g_map_mtx);
    if (g_dealloc_swizzled.count(cls)) return;
    Method m = class_getInstanceMethod(cls, sel_registerName("dealloc"));
    IMP old = method_getImplementation(m);
    method_setImplementation(m, (IMP)repl_buffer_dealloc);
    g_orig_dealloc_by_cls.emplace(cls, old);
    g_dealloc_swizzled.insert(cls);
}

// -------- class enumeration (covers Apple's private subclasses)
extern "C" __attribute__((visibility("default"))) void viz_swizzle_all_metal_classes(void) {
    Protocol *pDev  = objc_getProtocol("MTLDevice");
    Protocol *pHeap = objc_getProtocol("MTLHeap");

    SEL sel_dev_len      = sel_registerName("newBufferWithLength:options:");
    SEL sel_dev_bytes    = sel_registerName("newBufferWithBytes:length:options:");
    SEL sel_heap_len     = sel_registerName("newBufferWithLength:options:");

    int n = objc_getClassList(NULL, 0);
    if (n <= 0) return;
    
    // Safety: limit to reasonable number of classes to prevent hash table overflow
    if (n > 50000) {
        fprintf(stderr, "[ggml-viz] Warning: objc_getClassList returned %d classes, limiting to 50000\n", n);
        n = 50000;
    }
    
    Class *classes = (Class *)malloc(sizeof(Class) * n);
    n = objc_getClassList(classes, n);
    
    // Pre-reserve hash set to avoid rehashing (assume ~10% of classes need dealloc swizzling)
    g_dealloc_swizzled.reserve(std::min(n / 10 + 100, 5000));

    for (int i = 0; i < n; ++i) {
        Class cls = classes[i];
        if (!cls) continue;

        if (pDev && class_conformsToProtocol(cls, pDev)) {
            if (class_getInstanceMethod(cls, sel_dev_len))
                swizzle_method(cls, sel_dev_len, (IMP)repl_dev_newBufferWithLength_options, g_orig_dev_len_opts);
            if (class_getInstanceMethod(cls, sel_dev_bytes))
                swizzle_method(cls, sel_dev_bytes, (IMP)repl_dev_newBufferWithBytes_length_options, g_orig_dev_bytes_len_opts);
        }
        if (pHeap && class_conformsToProtocol(cls, pHeap)) {
            if (class_getInstanceMethod(cls, sel_heap_len))
                swizzle_method(cls, sel_heap_len, (IMP)repl_heap_newBufferWithLength_options, g_orig_heap_len_opts);
        }
    }
    free(classes);
}

// Metal swizzle initialization - now called from GGMLHook::start() for safer init
// Removed constructor to avoid early C++ initialization issues

// Hook accessor is now defined in ggml_hook.cpp to avoid duplication
#import "ggml_hook.hpp"
// ================== end ggml-viz Metal swizzles ====================