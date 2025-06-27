// src/instrumentation/ggml_hook.cpp
#include "ggml_hook.hpp"
#include "ggml-impl.h" // for ggml_cgraph, ggml_tensor
#include <thread>
#include <cstring>
#include <cassert>
#include <iostream>
#include <iomanip>

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
}

// Singleton implementation
GGMLHook& GGMLHook::instance() {
    static GGMLHook instance;
    printf("[DEBUG] GGMLHook::instance() called, returning %p\n", &instance);
    return instance;
}

void GGMLHook::configure(const HookConfig& config) {
    if (active_.load()) {
        std::cerr << "Warning: Cannot reconfigure while hook is active.\n";
        return;
    }
    config_ = config;
}

void GGMLHook::start() {
    if (active_.exchange(true)) {
        std::cerr << "Warning: GGMLHook is already active.\n";
        return; // Already active
    }

    start_time_ = std::chrono::steady_clock::now();
    event_count_ = 0;
    write_pos_ = 0;
    read_pos_ = 0;

    if (config_.write_to_file) {
        output_file_ = fopen(config_.output_filename.c_str(), "wb");
        if (!output_file_) {
            std::cerr << "Failed to open trace file: " << config_.output_filename << "\n";
            active_ = false;
            return;
        }

        const char magic[] = "GGMLVIZ1";
        fwrite(magic, 1, 8, output_file_);
        uint32_t version = 1;
        fwrite(&version, sizeof(version), 1, output_file_);
    }

    std::cout << "GGML Hook started. Output: " << config_.output_filename << "\n";
}

void GGMLHook::stop() {
    if (!active_.exchange(false)) {
        return;
    }
    
    printf("[DEBUG] GGMLHook::stop() called from somewhere\n");

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
    write_pos_ = 0;
    read_pos_ = 0;
}

std::vector<Event> GGMLHook::get_events_size(uint64_t timestamp_ns) {
    std::vector<Event> events;
    if (!active_.load()) return events;

    const size_t current_write = write_pos_.load(std::memory_order_acquire);
    size_t current_read = read_pos_.load(std::memory_order_relaxed);

    for (size_t i=current_read; i<current_write; ++i) {
        const size_t pos = i & (BUFFER_SIZE - 1);
        const Event& e = event_buffer_[pos];

        if (e.timestamp_ns <= timestamp_ns) {
            events.push_back(e);
        } else {
            break;
        }
    }

    read_pos_.store(current_read + events.size(), std::memory_order_release);
    return events;
}

GGMLHook::~GGMLHook() {
    printf("[DEBUG] GGMLHook destructor called\n");
    stop();
}

void GGMLHook::record_event(const Event& event) {
    if (!active_.load()) {
        printf("[DEBUG] record_event: not active\n");
        return;
    }

    if (event_count_.load() >= config_.max_events) {
        printf("[DEBUG] Event limit reached: %zu >= %zu, stopping trace\n", 
               event_count_.load(), config_.max_events);
        std::cerr << "Warning: Event limit reached, stopping trace\n";
        stop();
        return;
    }

    const size_t pos = write_pos_.fetch_add(1, std::memory_order_relaxed) & (BUFFER_SIZE - 1);
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        event_buffer_[pos] = event;
    }
    
    // Increment event count after storing the event
    event_count_.fetch_add(1, std::memory_order_relaxed);

    if (config_.write_to_file && (event_count_.load() & 0xFFF) == 0) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        flush_to_file();
    }
}

void GGMLHook::flush_to_file() {
    if (!output_file_) return;

    size_t current_write = write_pos_.load();
    size_t current_read = read_pos_.load();

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

    read_pos_ = current_read;
    fflush(output_file_);
}

void GGMLHook::on_graph_compute_begin(const ggml_cgraph* graph) {
    printf("[DEBUG] on_graph_compute_begin called, active: %d, enable_op_timing: %d\n", 
           active_.load(), config_.enable_op_timing);
    
    if (!active_.load() || !config_.enable_op_timing) return;

    std::cout << "[DEBUG] Graph compute begin, nodes: " << graph->n_nodes << "\n";

    Event event = {};
    event.type = EventType::GRAPH_COMPUTE_BEGIN;
    event.timestamp_ns = get_timestamp_ns();
    event.thread_id = get_thread_id();
    event.data.graph.graph_ptr = graph; // Assuming ggml_cgraph has a pointer field
    event.data.graph.n_nodes = graph->n_nodes;
    event.data.graph.n_threads = 1; // TODO: Get actual thread count
    event.label = nullptr;

    record_event(event);
}

void GGMLHook::on_graph_compute_end(const ggml_cgraph* graph) {
    if (!active_.load() || !config_.enable_op_timing) return;

    Event event = {};
    event.type = EventType::GRAPH_COMPUTE_END;
    event.timestamp_ns = get_timestamp_ns();
    event.thread_id = get_thread_id();
    event.data.graph.graph_ptr = graph; // Assuming ggml_cgraph has a pointer field
    event.data.graph.n_nodes = graph->n_nodes;
    event.data.graph.n_threads = 1; // TODO: Get actual thread count
    event.label = nullptr;

    record_event(event);
}

void GGMLHook::on_op_compute_begin(const ggml_tensor* tensor) {
    if (!active_.load() || !config_.enable_op_timing) return;

    if (!config_.op_types_to_trace.empty()) {
        // Check if this op type is in the filter list
        bool found = false;
        for (uint32_t op : config_.op_types_to_trace) {
            if (op == tensor->op) {
                found = true;
                break;
            }
        }
        if (!found) return;
    }

    std::cout << "[DEBUG] Op compute begin: " << tensor->name << " type: " << tensor->op << "\n";
        
    Event event = {};
    event.type = EventType::OP_COMPUTE_BEGIN;
    event.timestamp_ns = get_timestamp_ns();
    event.thread_id = get_thread_id();
    event.data.op.tensor_ptr = tensor;
    event.data.op.op_type = tensor->op;
    event.data.op.op_size = ggml_nbytes(tensor);
    event.label = config_.enable_tensor_names ? tensor->name : nullptr;
    
    record_event(event);
}

void GGMLHook::on_op_compute_end(const ggml_tensor* tensor) {
    if (!active_.load() || !config_.enable_op_timing) return;
    
    // Same filtering as begin
    if (!config_.op_types_to_trace.empty()) {
        bool found = false;
        for (uint32_t op : config_.op_types_to_trace) {
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
    event.data.op.op_size = ggml_nbytes(tensor);
    event.label = config_.enable_tensor_names ? tensor->name : nullptr;
    
    record_event(event);
}

// C-style hook functions that can be called from ggml
extern "C" {
    void ggml_viz_hook_graph_compute_begin(const ggml_cgraph* graph) {
        printf("[DEBUG] C wrapper called: ggml_viz_hook_graph_compute_begin\n");
        GGMLHook::instance().on_graph_compute_begin(graph);
    }

    void ggml_viz_hook_graph_compute_end(const ggml_cgraph* graph) {
        GGMLHook::instance().on_graph_compute_end(graph);
    }

    void ggml_viz_hook_op_compute_begin(const ggml_tensor* tensor) {
        GGMLHook::instance().on_op_compute_begin(tensor);
    }

    void ggml_viz_hook_op_compute_end(const ggml_tensor* tensor) {
        GGMLHook::instance().on_op_compute_end(tensor);
    }
}


// Installation helpers
bool install_ggml_hooks() {
    // Here we would typically patch the ggml functions to call our hooks
    // This is a placeholder as actual patching depends on the build system and linking
    std::cout << "Installing GGML hooks...\n";
    return true; // Assume success for now
}

bool uninstall_ggml_hooks() {
    // Here we would typically restore the original ggml functions
    // This is a placeholder as actual patching depends on the build system and linking
    std::cout << "Uninstalling GGML hooks...\n";
    return true; // Assume success for now
}

} // namespace ggml_viz