// src/server/data_collector.hpp
#pragma once

#include <vector>
#include <mutex>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include "ggml.h"

namespace ggml_viz {

enum class EventType {
    GRAPH_COMPUTE_BEGIN,
    GRAPH_COMPUTE_END,
    OP_COMPUTE_BEGIN,
    OP_COMPUTE_END
};

struct TraceEvent {
    EventType type;
    uint64_t timestamp_ns;
    std::thread::id thread_id;
    
    // For graph events
    const ggml_cgraph* graph_ptr = nullptr;
    
    // For op events
    const ggml_tensor* tensor_ptr = nullptr;
    enum ggml_op op_type = GGML_OP_NONE;
    const char* label = nullptr;
};

class DataCollector {
private:
    static DataCollector* instance_;
    std::mutex mutex_;
    std::vector<TraceEvent> events_;
    std::atomic<bool> enabled_{false};
    std::string output_filename_;

public:
    static DataCollector* getInstance() {
        if (!instance_) {
            instance_ = new DataCollector();
        }
        return instance_;
    }

    void enable(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        output_filename_ = filename;
        enabled_ = true;
        events_.clear();
        events_.reserve(100000); // Pre-allocate for performance
    }

    void disable() {
        enabled_ = false;
    }

    void record_event(const TraceEvent& event) {
        if (!enabled_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
    }

    void flush() {
        if (!enabled_ || output_filename_.empty()) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Write binary format
        std::ofstream out(output_filename_, std::ios::binary);
        if (!out) {
            printf("Failed to open trace file: %s\n", output_filename_.c_str());
            return;
        }
        
        // Write header
        const char magic[8] = "GGMLVIZ1";
        out.write(magic, 8);
        
        // Write number of events
        uint32_t num_events = events_.size();
        out.write(reinterpret_cast<const char*>(&num_events), sizeof(num_events));
        
        // Write events
        for (const auto& event : events_) {
            uint8_t event_type = static_cast<uint8_t>(event.type);
            out.write(reinterpret_cast<const char*>(&event_type), sizeof(event_type));
            out.write(reinterpret_cast<const char*>(&event.timestamp_ns), sizeof(event.timestamp_ns));
            
            // Write thread ID as uint64_t
            uint64_t tid = std::hash<std::thread::id>{}(event.thread_id);
            out.write(reinterpret_cast<const char*>(&tid), sizeof(tid));
            
            // Write operation-specific data
            if (event.type == EventType::OP_COMPUTE_BEGIN || event.type == EventType::OP_COMPUTE_END) {
                uint32_t op = static_cast<uint32_t>(event.op_type);
                out.write(reinterpret_cast<const char*>(&op), sizeof(op));
                
                // Write label
                uint32_t label_len = event.label ? strlen(event.label) : 0;
                out.write(reinterpret_cast<const char*>(&label_len), sizeof(label_len));
                if (label_len > 0) {
                    out.write(event.label, label_len);
                }
            }
        }
        
        out.close();
        printf("Wrote %zu events to trace file: %s\n", events_.size(), output_filename_.c_str());
    }

    size_t event_count() const {
        return events_.size();
    }
};

DataCollector* DataCollector::instance_ = nullptr;

} // namespace ggml_viz