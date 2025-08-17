// src/instrumentation/ggml_hook.hpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include <mutex>

struct ggml_tensor;
struct ggml_cgraph;
struct ggml_compute_params;
struct ggml_backend;

// Conditional compilation for function overrides
#ifndef GGML_VIZ_TEST_MODE
  #ifdef _WIN32
    #define GGML_VIZ_API __declspec(dllexport)
  #else
    #define GGML_VIZ_API __attribute__((visibility("default")))
  #endif
#else
  #define GGML_VIZ_API 
#endif

namespace ggml_viz {

enum class EventType : uint8_t {
    GRAPH_COMPUTE_BEGIN,
    GRAPH_COMPUTE_END,
    OP_COMPUTE_BEGIN,
    OP_COMPUTE_END,
    TENSOR_ALLOC,
    TENSOR_FREE,
    BARRIER_WAIT,
    THREAD_BEGIN,
    THREAD_FREE
};

struct Event {
    EventType type;
    uint64_t timestamp_ns; // nanoseconds since epoch
    uint32_t thread_id;

    union {
        struct {
            const void* tensor_ptr;
            uint32_t op_type;
            size_t op_size;
            const void* backend_ptr;
        } op;

        struct {
            const void* graph_ptr;
            uint32_t n_nodes;
            uint32_t n_threads;
            const void* backend_ptr;
        } graph;

        struct {
            const void* ptr;
            size_t size;
        } memory;
    } data;

    // Optional string data (e.g, tensor names)
    const char* label;
};

// Legacy HookConfig struct - deprecated in favor of ConfigManager
// Kept for backward compatibility
struct HookConfig {
    bool enable_op_timing = true;
    bool enable_memory_tracking = false;
    bool enable_thread_tracking = false;
    bool enable_tensor_names = true;

    // Output modes
    bool write_to_file = true;
    std::string output_filename = "ggml_trace.bin";

    // Filtering
    std::vector<uint32_t> op_types_to_trace; // Empty = trace all
    size_t max_events = 1000000;
};

class GGMLHook {
public:
    static GGMLHook& instance();
    
    // Config (deprecated - use ConfigManager instead)
    void configure(const HookConfig& config);

    // Control
    void start();
    void stop();
    bool is_active() const { return active_.load(); }

    // Stats
    size_t event_count() const { return event_count_.load(); }
    void reset_stats();    

    // Data access (for live viewers)
    std::vector<Event> get_events_size(uint64_t timestamp_ns);
    
    // Live data access for real-time visualization
    std::vector<Event> consume_available_events();
    Event* get_ring_buffer() { return event_buffer_; }
    size_t get_buffer_size() const { return BUFFER_SIZE; }
    size_t get_current_write_pos() const { return write_pos_.v.load(std::memory_order_acquire); }
    size_t get_current_read_pos() const { return read_pos_.v.load(std::memory_order_acquire); }
    size_t get_dropped_events() const { return dropped_events_.load(std::memory_order_relaxed); }

    void on_graph_compute_begin(const ggml_cgraph* graph, const ggml_backend* backend = nullptr);
    void on_graph_compute_end(const ggml_cgraph* graph, const ggml_backend* backend = nullptr);
    void on_op_compute_begin(const ggml_tensor* tensor, const ggml_backend* backend = nullptr);
    void on_op_compute_end(const ggml_tensor* tensor, const ggml_backend* backend = nullptr);
    
    // Memory tracking hooks
    void on_tensor_alloc(const ggml_tensor* tensor, size_t size, const ggml_backend* backend = nullptr);
    void on_tensor_free(const ggml_tensor* tensor, const ggml_backend* backend = nullptr);
    
    // Backend buffer tracking hooks
    void on_backend_buffer_alloc(void* buffer, size_t size);
    void on_backend_buffer_free(void* buffer);

    // Destructor flushes data
    ~GGMLHook();

private:
    GGMLHook(); // Constructor with environment variable initialization
    GGMLHook(const GGMLHook&) = delete;
    GGMLHook& operator=(const GGMLHook&) = delete;

    void record_event(const Event& event);
    void flush_to_file();

    std::atomic<bool> active_{false};
    std::atomic<size_t> event_count_{0};

    // Lock-free SPSC ring buffer
    // Cache-line aligned atomics to prevent false sharing  
    struct alignas(64) IndexPad {
        std::atomic<uint64_t> v{0};
        char _pad[64 - sizeof(std::atomic<uint64_t>)];
    };
    
    static constexpr size_t BUFFER_SIZE = 65536; // Must be pow of 2
    Event event_buffer_[BUFFER_SIZE];
    
    // head = producer write position, tail = consumer read position
    IndexPad write_pos_;  // head (producer)
    IndexPad read_pos_;   // tail (consumer) 
    
    // Dropped events counter for backpressure monitoring
    std::atomic<uint64_t> dropped_events_{0};
    
    std::mutex file_mutex_;

    FILE* output_file_ = nullptr;

    std::chrono::steady_clock::time_point start_time_;
};

// Note: GGML_VIZ_API is defined at the top of the file

extern "C" {
    GGML_VIZ_API void ggml_viz_hook_graph_compute_begin(const ggml_cgraph* graph, const ggml_backend* backend = nullptr);
    GGML_VIZ_API void ggml_viz_hook_graph_compute_end(const ggml_cgraph* graph, const ggml_backend* backend = nullptr);
    GGML_VIZ_API void ggml_viz_hook_op_compute_begin(const ggml_tensor* tensor, const ggml_backend* backend = nullptr);
    GGML_VIZ_API void ggml_viz_hook_op_compute_end(const ggml_tensor* tensor, const ggml_backend* backend = nullptr);
    
    // Memory tracking hooks
    GGML_VIZ_API void ggml_viz_hook_tensor_alloc(const ggml_tensor* tensor, size_t size, const ggml_backend* backend = nullptr);
    GGML_VIZ_API void ggml_viz_hook_tensor_free(const ggml_tensor* tensor, const ggml_backend* backend = nullptr);
    
    // Status functions for debugging
    GGML_VIZ_API bool ggml_viz_is_initialized();
    GGML_VIZ_API void ggml_viz_print_status();
}

GGML_VIZ_API bool install_ggml_hooks();
GGML_VIZ_API bool uninstall_ggml_hooks();

} // namespace ggml_viz