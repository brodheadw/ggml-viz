// src/utils/trace_reader.hpp
#pragma once

#include "instrumentation/ggml_hook.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ggml_viz {

class TraceReader {
public:
    struct TraceHeader {
        char magic[8]; // "GGMLVIZ1"
        uint32_t version;
    };

    explicit TraceReader(const std::string& filename);
    ~TraceReader();
    
    // Delete copy constructor and assignment operator since we manage a FILE* resource
    TraceReader(const TraceReader&) = delete;
    TraceReader& operator=(const TraceReader&) = delete;

    bool is_valid() const { return valid_; }
    size_t event_count() const { return events_.size(); }
    const std::vector<Event>& events() const { return events_; }

    // Analysis helpers
    std::vector<const Event*> get_graph_events() const;
    std::vector<const Event*> get_op_events_for_type(uint32_t op_type) const;
    uint64_t get_total_duration_ns() const;

    struct OpTiming { // Timing for a specific op type
        const Event* begin;
        const Event* end;
        uint64_t duration_ns;
        std::string name;
    };
    std::vector<OpTiming> get_op_timings() const;

private: 
    bool load_events();

    std::string filename_;
    FILE* file_ = nullptr;
    bool valid_ = false;
    TraceHeader header_;
    std::vector<Event> events_;
};

} // namespace ggml_viz