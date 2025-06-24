#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>

struct ggml_tensor;
struct ggml_cgraph;
struct ggml_compute_params;

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
        } op;

        struct {
            const void* graph_ptr;
            uint32_t n_nodes;
            uint32_t n_threads;
        } graph;

        struct {
            const void* ptr;
            size_t size;
        } memory;
    } data;

    // Optional string data (e.g, tensor names)
    const char* label;
};

// Hook configuration
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
    size_t max_events = 1000000; // Prevent runaway memory usage
};

class GGMLHook {
public:
    static GGMLHook& instance();
    
    // Config
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

    // Callbacks (called by our patched ggml functions)
    void on_graph_compute_begin(const ggml_cgraph* graph);
    void on_graph_compute_end(const ggml_cgraph* graph);
    void on_op_compute_begin(const ggml_tensor* tensor);
    void on_op_compute_end(const ggml_tensor* tensor);

    // Destructor flushes data
    ~GGMLHook();

private:
    GGMLHook() = default;
    GGMLHook(const GGMLHook&) = delete;
    GGMLHook& operator=(const GGMLHook&) = delete;

    void record_event(const Event& event);
    void flush_to_file();

    HookConfig config_;
    std::atomic<bool> active_{false};
    std::atomic<size_t> event_count_{0};

    // Ring buffer for events
    static constexpr size_t BUFFER_SIZE = 65536; // Must be pow of 2
    Event event_buffer_[BUFFER_SIZE];
    std::atomic<size_t> write_pos_{0};
    std::atomic<size_t> read_pos_{0};

    // File output
    FILE* output_file_ = nullptr;

    // Timing
    std::chrono::steady_clock::time_point start_time_;
};

// C-style functions that we'll inject into ggml
extern "C" {
    void ggml_viz_hook_graph_compute_begin(const ggml_cgraph* graph);
    void ggml_viz_hook_graph_compute_end(const ggml_cgraph* graph);
    void ggml_viz_hook_op_compute_begin(const ggml_tensor* tensor);
    void ggml_viz_hook_op_compute_end(const ggml_tensor* tensor);
}

bool install_ggml_hooks();
bool uninstall_ggml_hooks();

}