// ===============================================================================
// linux_interpose.cpp - Linux LD_PRELOAD interposition for GGML functions
// ===============================================================================

#if defined(__linux__) || defined(__unix__)

#include "ggml_hook.hpp"
#include "../ipc/ipc_common.hpp"
#include <ggml-backend.h>
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace ggml_viz {

// Function pointer types for original GGML functions
typedef enum ggml_status (*ggml_backend_sched_graph_compute_func_t)(
    ggml_backend_sched_t sched, 
    struct ggml_cgraph * graph
);

// Global function pointers to original implementations
static ggml_backend_sched_graph_compute_func_t original_ggml_backend_sched_graph_compute = nullptr;

// IPC shared memory region for event streaming
static std::unique_ptr<SharedMemoryRegion> g_shared_memory = nullptr;

// Initialize function pointers and shared memory
static void initialize_hooks() {
    static bool initialized = false;
    if (initialized) return;
    
    // Load original function symbols
    original_ggml_backend_sched_graph_compute = 
        (ggml_backend_sched_graph_compute_func_t)dlsym(RTLD_NEXT, "ggml_backend_sched_graph_compute");
    
    if (!original_ggml_backend_sched_graph_compute) {
        std::cerr << "ggml-viz: Warning: Could not find original ggml_backend_sched_graph_compute" << std::endl;
        return;
    }
    
    // Initialize shared memory if output is configured
    const char* output_file = std::getenv("GGML_VIZ_OUTPUT");
    if (output_file) {
        try {
            // Create 1MB shared memory region for event streaming
            g_shared_memory = SharedMemoryRegion::create("ggml_viz_events", 1024 * 1024, true);
            
            // Initialize the GGMLHook singleton
            GGMLHook::getInstance().initialize(output_file);
            
            if (std::getenv("GGML_VIZ_VERBOSE")) {
                std::cout << "ggml-viz: Linux interposition initialized successfully" << std::endl;
                std::cout << "ggml-viz: Output file: " << output_file << std::endl;
                std::cout << "ggml-viz: Shared memory region created" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "ggml-viz: Failed to initialize shared memory: " << e.what() << std::endl;
        }
    }
    
    initialized = true;
}

// Helper function to write event to shared memory
static void write_event_to_shared_memory(const Event& event) {
    if (g_shared_memory) {
        // Write event to shared memory ring buffer
        g_shared_memory->write(&event, sizeof(event));
    }
}

} // namespace ggml_viz

// ===============================================================================
// C function interposition - these override the original GGML functions
// ===============================================================================

extern "C" {

// Override ggml_backend_sched_graph_compute
enum ggml_status ggml_backend_sched_graph_compute(ggml_backend_sched_t sched, struct ggml_cgraph * graph) {
    using namespace ggml_viz;
    
    // Initialize hooks on first call
    initialize_hooks();
    
    // If we couldn't find the original function, fail gracefully
    if (!original_ggml_backend_sched_graph_compute) {
        std::cerr << "ggml-viz: Error: Original function not found" << std::endl;
        return GGML_STATUS_FAILED;
    }
    
    // Record graph compute begin event
    auto& hook = GGMLHook::getInstance();
    Event begin_event;
    begin_event.type = EventType::GRAPH_COMPUTE_BEGIN;
    begin_event.timestamp = hook.getCurrentTimestamp();
    begin_event.thread_id = hook.getCurrentThreadId();
    begin_event.data.graph_data.node_count = graph ? graph->n_nodes : 0;
    begin_event.data.graph_data.leaf_count = graph ? graph->n_leafs : 0;
    
    hook.recordEvent(begin_event);
    write_event_to_shared_memory(begin_event);
    
    // Call original function
    enum ggml_status result = original_ggml_backend_sched_graph_compute(sched, graph);
    
    // Record graph compute end event
    Event end_event;
    end_event.type = EventType::GRAPH_COMPUTE_END;
    end_event.timestamp = hook.getCurrentTimestamp();
    end_event.thread_id = hook.getCurrentThreadId();
    end_event.data.graph_data.node_count = graph ? graph->n_nodes : 0;
    end_event.data.graph_data.leaf_count = graph ? graph->n_leafs : 0;
    
    hook.recordEvent(end_event);
    write_event_to_shared_memory(end_event);
    
    return result;
}

// Constructor function - called when library is loaded
__attribute__((constructor))
void ggml_viz_init() {
    using namespace ggml_viz;
    
    if (std::getenv("GGML_VIZ_VERBOSE")) {
        std::cout << "ggml-viz: Linux interposition library loaded" << std::endl;
    }
    
    // Initialize hooks immediately
    initialize_hooks();
}

// Destructor function - called when library is unloaded
__attribute__((destructor))
void ggml_viz_cleanup() {
    using namespace ggml_viz;
    
    if (std::getenv("GGML_VIZ_VERBOSE")) {
        std::cout << "ggml-viz: Linux interposition library unloaded" << std::endl;
    }
    
    // Cleanup shared memory and flush any remaining events
    if (g_shared_memory) {
        g_shared_memory.reset();
    }
    
    // Finalize the hook system
    GGMLHook::getInstance().finalize();
}

} // extern "C"

#endif // __linux__ || __unix__