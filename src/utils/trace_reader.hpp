// src/utils/trace_reader.hpp
#pragma once

#include "instrumentation/ggml_hook.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace ggml_viz {

class TraceReader {
public:
    struct TraceHeader {
        char magic[8]; // "GGMLVIZ1"
        uint32_t version;
    };

    explicit TraceReader(const std::string& filename);
    ~TraceReader();

    TraceReader(const TraceReader&) = delete;
    TraceReader& operator=(const TraceReader&) = delete;

    bool is_valid() const { return valid_; }
    size_t event_count() const { return events_.size(); }
    const std::vector<Event>& events() const { return events_; }

    // Analysis helpers
    std::vector<const Event*> get_graph_events() const;
    std::vector<const Event*> get_op_events_for_type(uint32_t op_type) const;
    uint64_t get_total_duration_ns() const;
    
    // Memory analysis helpers
    std::vector<const Event*> get_memory_events() const;
    std::vector<const Event*> get_alloc_events() const;
    std::vector<const Event*> get_free_events() const;
    size_t get_peak_memory_usage() const;
    size_t get_current_memory_usage() const;

    struct OpTiming { // Timing for a specific op type
        const Event* begin;
        const Event* end;
        uint64_t duration_ns;
        std::string name;
    };
    std::vector<OpTiming> get_op_timings() const;
    
    struct MemoryStats {
        uint64_t total_allocations = 0;
        uint64_t total_frees = 0;
        uint64_t bytes_allocated = 0;
        uint64_t bytes_freed = 0;
        uint64_t current_usage = 0;
        uint64_t peak_usage = 0;
        uint64_t leaked_bytes = 0;
        uint64_t first_alloc_time = 0;
        uint64_t last_free_time = 0;
    };
    MemoryStats get_memory_stats() const;
    
    // Memory curve generation with proper FREE size handling
    std::vector<std::pair<uint64_t, uint64_t>> get_memory_curve_bytes() const;

private: 
    bool load_events();
    void update_memory_stats() const;

    std::string filename_;
    FILE* file_ = nullptr;
    bool valid_ = false;
    TraceHeader header_;
    std::vector<Event> events_;
    
    // Cached memory analysis state
    mutable bool memory_stats_dirty_ = true;
    mutable MemoryStats cached_memory_stats_;
    mutable std::unordered_map<const void*, uint64_t> allocations_;
    mutable std::unordered_set<const void*> freed_pointers_; // Debug: detect double-free
    mutable uint64_t current_usage_ = 0;
    
    // Cached curve data
    mutable uint64_t cached_current_allocated_ = 0;
    mutable uint64_t cached_peak_allocated_ = 0;
};

} // namespace ggml_viz