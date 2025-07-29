// src/instrumentation/ggml_hook.cpp
#include "ggml_hook.hpp"
#include "../utils/config.hpp"
#include "ggml-impl.h" // for ggml_cgraph, ggml_tensor
#include "ggml-backend.h" // for ggml_backend_t
#include <thread>
#include <cstring>
#include <cassert>
#include <iostream>
#include <iomanip>
#ifdef _WIN32
#include <io.h>     // for _commit, _fileno
#else
#include <dlfcn.h>  // for dlopen, dlsym, dlclose
#include <unistd.h> // for fsync
#include <fcntl.h>  // for fcntl
#endif

namespace ggml_viz {

namespace {
    uint32_t get_thread_id() {
        auto id = std::this_thread::get_id();
        return std::hash<std::thread::id>{}(id) & 0xFFFFFFFF; // Ensure 32-bit
    }

    uint64_t get_timestamp_ns() {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(
            steady_clock::now().time_since_epoch()
        ).count();
    }
    
    // Simple implementation of ggml_nbytes to avoid linking dependencies
    size_t ggml_nbytes_simple(const ggml_tensor* tensor) {
        if (!tensor) return 0;
        
        // Calculate total elements
        size_t total_elements = 1;
        for (int i = 0; i < 4; i++) {
            total_elements *= tensor->ne[i];
        }
        
        // Estimate bytes per element based on type
        size_t element_size = 4; // Default to float32
        switch (tensor->type) {
            case GGML_TYPE_F32: element_size = 4; break;
            case GGML_TYPE_F16: element_size = 2; break;
            case GGML_TYPE_Q4_0: element_size = 1; break; // Approximation for quantized
            case GGML_TYPE_Q4_1: element_size = 1; break;
            case GGML_TYPE_Q5_0: element_size = 1; break;
            case GGML_TYPE_Q5_1: element_size = 1; break;
            case GGML_TYPE_Q8_0: element_size = 1; break;
            case GGML_TYPE_Q8_1: element_size = 1; break;
            case GGML_TYPE_I8:   element_size = 1; break;
            case GGML_TYPE_I16:  element_size = 2; break;
            case GGML_TYPE_I32:  element_size = 4; break;
            default: element_size = 4; break;
        }
        
        return total_elements * element_size;
    }
}

// Private constructor - configuration now handled by ConfigManager
GGMLHook::GGMLHook() : event_buffer_{}, write_pos_{}, read_pos_{} {
    // Configuration is now managed by ConfigManager singleton
    // No initialization needed here - config is loaded via CLI/file
}

// Singleton implementation
GGMLHook& GGMLHook::instance() {
    static GGMLHook instance;
    return instance;
}

void GGMLHook::configure(const HookConfig& config) {
    if (active_.load()) {
        std::cerr << "Warning: Cannot reconfigure while hook is active.\n";
        return;
    }
    // Legacy method - now handled by ConfigManager
    std::cerr << "Warning: GGMLHook::configure() is deprecated. Use ConfigManager::load_with_precedence() instead.\n";
}

void GGMLHook::start() {
    // Get current configuration from ConfigManager
    auto& config_mgr = ConfigManager::instance();
    
    // Load default config if not already loaded
    if (!config_mgr.is_loaded()) {
        // Try to load from environment-specified config file or defaults
        const char* config_file_env = getenv("GGML_VIZ_CONFIG");
        config_mgr.load_with_precedence("", config_file_env ? config_file_env : "", "ggml-viz.json");
    }
    
    auto config = config_mgr.get();
    
    // Check if hooks are disabled via config
    if (!config->instrumentation.enable_op_timing && !config->instrumentation.enable_memory_tracking) {
        std::cout << "GGML Viz hooks disabled by configuration.\n";
        return;
    }

    if (active_.exchange(true)) {
        std::cerr << "Warning: GGMLHook is already active.\n";
        return; // Already active
    }

    start_time_ = std::chrono::steady_clock::now();
    event_count_ = 0;
    write_pos_.v.store(0, std::memory_order_relaxed);
    read_pos_.v.store(0, std::memory_order_relaxed);
    dropped_events_.store(0, std::memory_order_relaxed);

    if (config->output.write_to_file) {
        output_file_ = fopen(config->output.filename.c_str(), "wb");
        if (!output_file_) {
            std::cerr << "Failed to open trace file: " << config->output.filename << "\n";
            active_ = false;
            return;
        }

        const char magic[] = "GGMLVIZ1";
        fwrite(magic, 1, 8, output_file_);
        uint32_t version = 1;
        fwrite(&version, sizeof(version), 1, output_file_);
        
        // Ensure header is written to disk immediately
        fflush(output_file_);
#ifndef _WIN32
        fsync(fileno(output_file_));
#else
        _commit(_fileno(output_file_));
#endif
    }

    std::cout << "GGML Hook started. Output: " << config->output.filename << "\n";
    if (config->logging.level == ConfigLogLevel::DEBUG) {
        std::cout << "GGML Viz verbose mode enabled.\n";
    }
}

void GGMLHook::stop() {
    if (!active_.exchange(false)) {
        return;
    }

    if (output_file_) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        flush_to_file();
        fclose(output_file_);
        output_file_ = nullptr;
    }

    std::cout << "GGML Hook stopped. Recorded " << event_count_ << " events.\n";
}

void GGMLHook::reset_stats() {
    if (active_.load()) {
        std::cerr << "Warning: Cannot reset stats while hook is active.\n";
        return;
    }
    event_count_ = 0;
    write_pos_.v.store(0, std::memory_order_relaxed);
    read_pos_.v.store(0, std::memory_order_relaxed);
    dropped_events_.store(0, std::memory_order_relaxed);
}

std::vector<Event> GGMLHook::get_events_size(uint64_t timestamp_ns) {
    std::vector<Event> events;
    if (!active_.load()) return events;

    const uint64_t head = write_pos_.v.load(std::memory_order_acquire);
    uint64_t tail = read_pos_.v.load(std::memory_order_relaxed);

    for (uint64_t i = tail; i < head; ++i) {
        const size_t pos = i & (BUFFER_SIZE - 1);
        const Event& e = event_buffer_[pos];

        if (e.timestamp_ns <= timestamp_ns) {
            events.push_back(e);
        } else {
            break;
        }
    }

    read_pos_.v.store(tail + events.size(), std::memory_order_relaxed);
    return events;
}

std::vector<Event> GGMLHook::consume_available_events() {
    std::vector<Event> events;
    if (!active_.load()) return events;

    // Consumer (tail=read_pos_, head=write_pos_)
    const uint64_t tail = read_pos_.v.load(std::memory_order_relaxed);    // own counter
    const uint64_t head = write_pos_.v.load(std::memory_order_acquire);   // see producer updates

    // Calculate available events using monotonic counters
    const uint64_t available = head - tail;
    if (available == 0) return events;

    events.reserve(available);

    // Copy all available events
    for (uint64_t i = tail; i < head; ++i) {
        const size_t pos = i & (BUFFER_SIZE - 1);
        events.push_back(event_buffer_[pos]);
    }

    // Update read position with release fence - signals space available to producer
    read_pos_.v.store(head, std::memory_order_relaxed);  // can be relaxed per feedback
    return events;
}

GGMLHook::~GGMLHook() {
    stop();
}

void GGMLHook::record_event(const Event& event) {
    if (!active_.load()) {
        return;
    }

    // Get current config for max_events check (lock-free hot path)
    auto config = ConfigManager::instance().get();
    if (event_count_.load() >= config->instrumentation.max_events) {
        std::cerr << "Warning: Event limit reached, stopping trace\n";
        stop();
        return;
    }

    // SPSC lock-free implementation with proper memory ordering
    // Producer (head=write_pos_, tail=read_pos_)
    // We use head=producer, tail=consumer naming convention
    const uint64_t head = write_pos_.v.load(std::memory_order_relaxed);  // own counter
    const uint64_t tail = read_pos_.v.load(std::memory_order_acquire);   // see consumer updates
    
    // Check if buffer full - we leave one slot empty to distinguish full/empty states
    // This is the "one empty slot" rule for lock-free ring buffers
    const uint64_t next = head + 1;
    if ((next & (BUFFER_SIZE - 1)) == (tail & (BUFFER_SIZE - 1))) {
        // Buffer full, drop event and increment counter for backpressure monitoring
        dropped_events_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    
    // Write event to buffer
    event_buffer_[head & (BUFFER_SIZE - 1)] = event;
    
    // Publish with release fence - signals data availability to consumer
    write_pos_.v.store(next, std::memory_order_release);
    
    // Increment event count after storing the event
    size_t new_count = event_count_.fetch_add(1, std::memory_order_relaxed) + 1;

    if (config->output.write_to_file && (new_count & 0xFFF) == 0) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        flush_to_file();
    }
}

void GGMLHook::flush_to_file() {
    if (!output_file_) return;

    uint64_t current_write = write_pos_.v.load(std::memory_order_acquire);
    uint64_t current_read = read_pos_.v.load(std::memory_order_relaxed);

    while (current_read < current_write) {
        size_t pos = current_read & (BUFFER_SIZE - 1);
        const Event& e = event_buffer_[pos];

        // Write event (binary format for speed)
        fwrite(&e.type, sizeof(e.type), 1, output_file_);
        fwrite(&e.timestamp_ns, sizeof(e.timestamp_ns), 1, output_file_);
        fwrite(&e.thread_id, sizeof(e.thread_id), 1, output_file_);
        fwrite(&e.data, sizeof(e.data), 1, output_file_);

        // Write label if present
        uint8_t has_label = (e.label != nullptr) ? 1 : 0;
        fwrite(&has_label, 1, 1, output_file_);
        if (has_label) {
            uint32_t label_len = strlen(e.label);
            fwrite(&label_len, sizeof(label_len), 1, output_file_);
            fwrite(e.label, 1, label_len, output_file_);
        }

        current_read++;
    }

    read_pos_.v.store(current_read, std::memory_order_relaxed);
    fflush(output_file_);
    
#ifdef __APPLE__
    int fd = fileno(output_file_);
    fcntl(fd, F_FULLFSYNC, 0);
#elif defined(_WIN32)
    _commit(_fileno(output_file_));
#else
    int fd = fileno(output_file_);
    fsync(fd);
#endif
}

void GGMLHook::on_graph_compute_begin(const ggml_cgraph* graph, const ggml_backend* backend) {
    if (!active_.load()) return;
    
    auto config = ConfigManager::instance().get();
    if (!config->instrumentation.enable_op_timing) return;

    // Add NULL check for graph pointer
    int n_nodes = (graph != nullptr) ? graph->n_nodes : 0;

    Event event = {};
    event.type = EventType::GRAPH_COMPUTE_BEGIN;
    event.timestamp_ns = get_timestamp_ns();
    event.thread_id = get_thread_id();
    event.data.graph.graph_ptr = graph;
    event.data.graph.n_nodes = n_nodes;
    event.data.graph.n_threads = 1; // TODO: Get actual thread count
    event.data.graph.backend_ptr = backend;
    event.label = nullptr;

    record_event(event);
}

void GGMLHook::on_graph_compute_end(const ggml_cgraph* graph, const ggml_backend* backend) {
    if (!active_.load()) return;
    
    auto config = ConfigManager::instance().get();
    if (!config->instrumentation.enable_op_timing) return;

    // Add NULL check for graph pointer
    int n_nodes = (graph != nullptr) ? graph->n_nodes : 0;

    Event event = {};
    event.type = EventType::GRAPH_COMPUTE_END;
    event.timestamp_ns = get_timestamp_ns();
    event.thread_id = get_thread_id();
    event.data.graph.graph_ptr = graph;
    event.data.graph.n_nodes = n_nodes;
    event.data.graph.n_threads = 1; // TODO: Get actual thread count
    event.data.graph.backend_ptr = backend;
    event.label = nullptr;

    record_event(event);
    
    // Flush to file immediately after each graph computation for live mode
    if (config->output.write_to_file) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        flush_to_file();
    }
}

void GGMLHook::on_op_compute_begin(const ggml_tensor* tensor, const ggml_backend* backend) {
    if (!active_.load()) return;
    
    auto config = ConfigManager::instance().get();
    if (!config->instrumentation.enable_op_timing) return;

    // Add NULL check for tensor pointer
    if (tensor == nullptr) return;

    if (!config->instrumentation.op_types_to_trace.empty()) {
        // Check if this op type is in the filter list
        bool found = false;
        for (uint32_t op : config->instrumentation.op_types_to_trace) {
            if (op == tensor->op) {
                found = true;
                break;
            }
        }
        if (!found) return;
    }

    std::cout << "[DEBUG] Op compute begin: " << tensor->name << " type: " << tensor->op << ", backend: " << (backend ? "yes" : "no") << "\n";
        
    Event event = {};
    event.type = EventType::OP_COMPUTE_BEGIN;
    event.timestamp_ns = get_timestamp_ns();
    event.thread_id = get_thread_id();
    event.data.op.tensor_ptr = tensor;
    event.data.op.op_type = tensor->op;
    event.data.op.op_size = ggml_nbytes_simple(tensor);
    event.data.op.backend_ptr = backend;
    event.label = config->instrumentation.record_tensor_names ? tensor->name : nullptr;
    
    record_event(event);
}

void GGMLHook::on_op_compute_end(const ggml_tensor* tensor, const ggml_backend* backend) {
    if (!active_.load()) return;
    
    auto config = ConfigManager::instance().get();
    if (!config->instrumentation.enable_op_timing) return;
    
    // Add NULL check for tensor pointer
    if (tensor == nullptr) return;
    
    // Same filtering as begin
    if (!config->instrumentation.op_types_to_trace.empty()) {
        bool found = false;
        for (uint32_t op : config->instrumentation.op_types_to_trace) {
            if (op == tensor->op) {
                found = true;
                break;
            }
        }
        if (!found) return;
    }
    
    Event event = {};
    event.type = EventType::OP_COMPUTE_END;
    event.timestamp_ns = get_timestamp_ns();
    event.thread_id = get_thread_id();
    event.data.op.tensor_ptr = tensor;
    event.data.op.op_type = tensor->op;
    event.data.op.op_size = ggml_nbytes_simple(tensor);
    event.data.op.backend_ptr = backend;
    event.label = config->instrumentation.record_tensor_names ? tensor->name : nullptr;
    
    record_event(event);
}

// C-style hook functions that can be called from ggml
extern "C" {
    void ggml_viz_hook_graph_compute_begin(const ggml_cgraph* graph, const ggml_backend* backend) {
        GGMLHook::instance().on_graph_compute_begin(graph, backend);
    }

    void ggml_viz_hook_graph_compute_end(const ggml_cgraph* graph, const ggml_backend* backend) {
        GGMLHook::instance().on_graph_compute_end(graph, backend);
    }

    void ggml_viz_hook_op_compute_begin(const ggml_tensor* tensor, const ggml_backend* backend) {
        GGMLHook::instance().on_op_compute_begin(tensor, backend);
    }

    void ggml_viz_hook_op_compute_end(const ggml_tensor* tensor, const ggml_backend* backend) {
        GGMLHook::instance().on_op_compute_end(tensor, backend);
    }
}

// Function pointers to original implementations
static enum ggml_status (*original_backend_graph_compute)(ggml_backend_t, struct ggml_cgraph*) = nullptr;
static void (*original_graph_compute)(struct ggml_context*, struct ggml_cgraph*) = nullptr;
static enum ggml_status (*original_graph_compute_with_ctx)(struct ggml_context*, struct ggml_cgraph*, int) = nullptr;
static enum ggml_status (*original_metal_graph_compute)(ggml_backend_t, struct ggml_cgraph*) = nullptr;
static bool hooks_initialized = false;

// Our intercepted functions - only for shared library builds (not static library or test builds)
#if !defined(GGML_VIZ_TEST_MODE) && defined(GGML_VIZ_SHARED_BUILD)
extern "C" {
    // Override ggml_backend_graph_compute
    GGML_VIZ_API enum ggml_status ggml_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cgraph) {
        auto& hook = GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] Intercepted ggml_backend_graph_compute, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, backend);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], backend);
                }
            }
        }
        
        // Call the original function
        enum ggml_status result = GGML_STATUS_FAILED;
        
        // Initialize function pointers if needed
        if (!hooks_initialized) {
            install_ggml_hooks();
            hooks_initialized = true;
        }
        
        if (original_backend_graph_compute) {
            result = original_backend_graph_compute(backend, cgraph);
        } else {
            printf("[GGML_VIZ] Warning: No original ggml_backend_graph_compute function found\n");
            // For now, just return success to avoid breaking the application
            result = GGML_STATUS_SUCCESS;
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
    
    // Alternative override for older GGML versions that might use ggml_graph_compute
    GGML_VIZ_API void ggml_graph_compute(struct ggml_context* ctx, struct ggml_cgraph* cgraph) {
        auto& hook = GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] Intercepted ggml_graph_compute, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, nullptr);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], nullptr);
                }
            }
        }
        
        // Call the original function
        
        // Initialize function pointers if needed
        if (!hooks_initialized) {
            install_ggml_hooks();
            hooks_initialized = true;
        }
        
        if (original_graph_compute) {
            original_graph_compute(ctx, cgraph);
        } else {
            printf("[GGML_VIZ] Warning: No original ggml_graph_compute function found\n");
            // Continue without calling original - this allows pure instrumentation mode
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
    
    // Override for ggml_graph_compute_with_ctx (commonly used in tests)
    GGML_VIZ_API enum ggml_status ggml_graph_compute_with_ctx(struct ggml_context* ctx, struct ggml_cgraph* cgraph, int n_threads) {
        auto& hook = GGMLHook::instance();
        
        if (hook.is_active()) {
            printf("[DEBUG] Intercepted ggml_graph_compute_with_ctx, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, nullptr);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], nullptr);
                }
            }
        }
        
        // Call the original function
        enum ggml_status result = GGML_STATUS_SUCCESS;
        
        // Initialize function pointers if needed
        if (!hooks_initialized) {
            install_ggml_hooks();
            hooks_initialized = true;
        }
        
        if (original_graph_compute_with_ctx) {
            result = original_graph_compute_with_ctx(ctx, cgraph, n_threads);
        } else {
            printf("[GGML_VIZ] Warning: No original ggml_graph_compute_with_ctx function found\n");
            // Continue without calling original - this allows pure instrumentation mode
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
    
    // Override ggml_backend_metal_graph_compute for Metal backend GPU work
    GGML_VIZ_API enum ggml_status ggml_backend_metal_graph_compute(ggml_backend_t backend, struct ggml_cgraph* cgraph) {
        auto& hook = GGMLHook::instance();
        
        // Auto-start hooks if GGML_VIZ_OUTPUT is set but hooks aren't active
        if (!hook.is_active() && getenv("GGML_VIZ_OUTPUT")) {
            printf("[GGML_VIZ] Auto-starting hooks due to GGML_VIZ_OUTPUT environment variable\n");
            hook.start();
        }
        
        if (hook.is_active()) {
            printf("[DEBUG] Intercepted ggml_backend_metal_graph_compute, nodes: %d\n", cgraph->n_nodes);
            hook.on_graph_compute_begin(cgraph, backend);
            
            // Call each node's begin hook
            for (int i = 0; i < cgraph->n_nodes; i++) {
                if (cgraph->nodes[i]) {
                    hook.on_op_compute_begin(cgraph->nodes[i], backend);
                }
            }
        }
        
        // Call the original function
        enum ggml_status result = GGML_STATUS_SUCCESS; // Default success
        
        // Initialize function pointers if needed
        if (!hooks_initialized) {
            install_ggml_hooks();
            hooks_initialized = true;
        }
        
        if (original_metal_graph_compute) {
            result = original_metal_graph_compute(backend, cgraph);
        } else {
            printf("[GGML_VIZ] Warning: No original ggml_backend_metal_graph_compute function found\n");
            // Continue without calling original - this allows pure instrumentation mode
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
}
#endif // !GGML_VIZ_TEST_MODE

// Installation helpers
bool install_ggml_hooks() {
    printf("[GGML_VIZ] Installing GGML function interception hooks...\n");
    
    // The function interception happens automatically when our shared library
    // overrides the weak symbols. This function just confirms setup.
    
    #ifndef _WIN32
    // Try to locate original functions for fallback
    void* handle = dlopen(NULL, RTLD_LAZY);
    if (handle) {
        // Look for the original functions in case they're not weak symbols
        auto backend_func = reinterpret_cast<enum ggml_status (*)(ggml_backend_t, struct ggml_cgraph*)>
            (dlsym(handle, "ggml_backend_graph_compute"));
        auto graph_func = reinterpret_cast<void (*)(struct ggml_context*, struct ggml_cgraph*)>
            (dlsym(handle, "ggml_graph_compute"));
        auto graph_with_ctx_func = reinterpret_cast<enum ggml_status (*)(struct ggml_context*, struct ggml_cgraph*, int)>
            (dlsym(handle, "ggml_graph_compute_with_ctx"));
        auto metal_func = reinterpret_cast<enum ggml_status (*)(ggml_backend_t, struct ggml_cgraph*)>
            (dlsym(handle, "ggml_backend_metal_graph_compute"));

        // Only store if they're different from our overrides (shared library builds only)
#if !defined(GGML_VIZ_TEST_MODE) && defined(GGML_VIZ_SHARED_BUILD)
        if (backend_func && backend_func != ggml_backend_graph_compute) {
            original_backend_graph_compute = backend_func;
            printf("[GGML_VIZ] Found original ggml_backend_graph_compute\n");
        }
        if (graph_func && graph_func != ggml_graph_compute) {
            original_graph_compute = graph_func;
            printf("[GGML_VIZ] Found original ggml_graph_compute\n");
        }
        if (graph_with_ctx_func && graph_with_ctx_func != ggml_graph_compute_with_ctx) {
            original_graph_compute_with_ctx = graph_with_ctx_func;
            printf("[GGML_VIZ] Found original ggml_graph_compute_with_ctx\n");
        }
        if (metal_func && metal_func != ggml_backend_metal_graph_compute) {
            original_metal_graph_compute = metal_func;
            printf("[GGML_VIZ] Found original ggml_backend_metal_graph_compute\n");
        }
#else
        // In test mode, just store the functions we find
        if (backend_func) {
            original_backend_graph_compute = backend_func;
            printf("[GGML_VIZ] Found ggml_backend_graph_compute (test mode)\n");
        }
        if (graph_func) {
            original_graph_compute = graph_func;
            printf("[GGML_VIZ] Found ggml_graph_compute (test mode)\n");
        }
        if (graph_with_ctx_func) {
            original_graph_compute_with_ctx = graph_with_ctx_func;
            printf("[GGML_VIZ] Found ggml_graph_compute_with_ctx (test mode)\n");
        }
        if (metal_func) {
            original_metal_graph_compute = metal_func;
            printf("[GGML_VIZ] Found ggml_backend_metal_graph_compute (test mode)\n");
        }
#endif
        
        dlclose(handle);
    }
    #endif
    
    printf("[GGML_VIZ] GGML function hooks installed successfully\n");
    return true;
}

bool uninstall_ggml_hooks() {
    printf("[GGML_VIZ] Uninstalling GGML hooks...\n");
    
    // Reset function pointers
    original_backend_graph_compute = nullptr;
    original_graph_compute = nullptr;
    
    printf("[GGML_VIZ] GGML hooks uninstalled\n");
    return true;
}

} // namespace ggml_viz

// C wrapper functions for Objective-C interposer
extern "C" {
    void* ggml_viz_get_hook_instance(void) {
        return &ggml_viz::GGMLHook::instance();
    }
    
    int ggml_viz_hook_is_active(void* hook) {
        return static_cast<ggml_viz::GGMLHook*>(hook)->is_active() ? 1 : 0;
    }
    
    void ggml_viz_hook_start(void* hook) {
        static_cast<ggml_viz::GGMLHook*>(hook)->start();
    }
    
    void ggml_viz_hook_on_graph_compute_begin(void* hook, const struct ggml_cgraph* graph, const void* backend) {
        static_cast<ggml_viz::GGMLHook*>(hook)->on_graph_compute_begin(graph, static_cast<const ggml_backend*>(backend));
    }
    
    void ggml_viz_hook_on_graph_compute_end(void* hook, const struct ggml_cgraph* graph, const void* backend) {
        static_cast<ggml_viz::GGMLHook*>(hook)->on_graph_compute_end(graph, static_cast<const ggml_backend*>(backend));
    }
}