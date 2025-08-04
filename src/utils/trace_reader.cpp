// src/utils/trace_reader.cpp
#include "trace_reader.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <cstdlib>

namespace ggml_viz {

TraceReader::TraceReader(const std::string& filename) : filename_(filename) {
    file_ = fopen(filename_.c_str(), "rb");
    if (!file_) {
        std::cerr << "Failed to open trace file: " << filename_ << "\n";
        return;
    }

    // Read header
    if (fread(&header_, sizeof(header_), 1, file_) != 1) {
        std::cerr << "Failed to read trace header\n";
        fclose(file_);
        file_ = nullptr;
        return;
    }

    // Verify magic
    if (strncmp(header_.magic, "GGMLVIZ1", 8) != 0) {
        std::cerr << "Invalid trace file magic\n";
        fclose(file_);
        file_ = nullptr;
        return;
    }

    valid_ = load_events();
    if (valid_) {
        // Count actual memory events for better sizing
        size_t memory_event_count = 0;
        for (const auto& event : events_) {
            if (event.type == EventType::TENSOR_ALLOC || event.type == EventType::TENSOR_FREE) {
                memory_event_count++;
            }
        }
        
        // Reserve hash maps - assume 50% allocs, 50% frees
        size_t estimated_allocs = std::max(memory_event_count / 2, size_t(1024));
        allocations_.reserve(estimated_allocs);
        freed_pointers_.reserve(estimated_allocs);
    }
}

TraceReader::~TraceReader() {
    if (file_) {
        fclose(file_);
    }
}

bool TraceReader::load_events() {
    events_.clear();

    while (!feof(file_)) {
        Event event;

        if (fread(&event.type, sizeof(event.type), 1, file_) != 1) break;
        if (fread(&event.timestamp_ns, sizeof(event.timestamp_ns), 1, file_) != 1) break;
        if (fread(&event.thread_id, sizeof(event.thread_id), 1, file_) != 1) break;
        if (fread(&event.data, sizeof(event.data), 1, file_) != 1) break;

        uint8_t has_label;
        if (fread(&has_label, 1, 1, file_) != 1) break;

        if (has_label) {
            uint32_t label_len;
            if (fread(&label_len, sizeof(label_len), 1, file_) != 1) break;

            // TODO: Implement string pool for labels
            std::vector<char> label_buf(label_len + 1);
            if (fread(label_buf.data(), 1, label_len, file_) != label_len) break;
            label_buf[label_len] = '\0';

            event.label = strdup(label_buf.data()); // TODO: Use a string pool instead of strdup to manage string lifetime properly
        } else {
            event.label = nullptr;
        }

        events_.push_back(event);
    }
    
    return true;
}

std::vector<const Event*> TraceReader::get_graph_events() const {
    std::vector<const Event*> result;
    for (const auto& event : events_) {
        if (event.type == EventType::GRAPH_COMPUTE_BEGIN || 
            event.type == EventType::GRAPH_COMPUTE_END) {
            result.push_back(&event);
        }
    }
    return result;
}

std::vector<const Event*> TraceReader::get_op_events_for_type(uint32_t op_type) const {
    std::vector<const Event*> result;
    for (const auto& event : events_) {
        if ((event.type == EventType::OP_COMPUTE_BEGIN || 
             event.type == EventType::OP_COMPUTE_END) &&
            event.data.op.op_type == op_type) {
            result.push_back(&event);
        }
    }
    return result;
}

uint64_t TraceReader::get_total_duration_ns() const {
    if (events_.size() < 2) return 0;
    return events_.back().timestamp_ns - events_.front().timestamp_ns;
}

std::vector<TraceReader::OpTiming> TraceReader::get_op_timings() const {
    std::vector<OpTiming> timings;
    std::unordered_map<const void*, const Event*> pending_ops;
    
    for (const auto& event : events_) {
        if (event.type == EventType::OP_COMPUTE_BEGIN) {
            pending_ops[event.data.op.tensor_ptr] = &event;
        } else if (event.type == EventType::OP_COMPUTE_END) {
            auto it = pending_ops.find(event.data.op.tensor_ptr);
            if (it != pending_ops.end()) {
                OpTiming timing;
                timing.begin = it->second;
                timing.end = &event;
                timing.duration_ns = event.timestamp_ns - it->second->timestamp_ns;
                timing.name = event.label ? event.label : "unnamed";
                timings.push_back(timing);
                pending_ops.erase(it);
            }
        }
    }
    
    // Sort by duration (longest first)
    std::sort(timings.begin(), timings.end(), 
              [](const OpTiming& a, const OpTiming& b) {
                  return a.duration_ns > b.duration_ns;
              });
    
    return timings;
}

std::vector<const Event*> TraceReader::get_memory_events() const {
    std::vector<const Event*> memory_events;
    for (const auto& event : events_) {
        if (event.type == EventType::TENSOR_ALLOC || event.type == EventType::TENSOR_FREE) {
            memory_events.push_back(&event);
        }
    }
    return memory_events;
}

std::vector<const Event*> TraceReader::get_alloc_events() const {
    std::vector<const Event*> alloc_events;
    for (const auto& event : events_) {
        if (event.type == EventType::TENSOR_ALLOC) {
            alloc_events.push_back(&event);
        }
    }
    return alloc_events;
}

std::vector<const Event*> TraceReader::get_free_events() const {
    std::vector<const Event*> free_events;
    for (const auto& event : events_) {
        if (event.type == EventType::TENSOR_FREE) {
            free_events.push_back(&event);
        }
    }
    return free_events;
}

size_t TraceReader::get_peak_memory_usage() const {
    if (memory_stats_dirty_) {
        update_memory_stats();
    }
    return static_cast<size_t>(cached_memory_stats_.peak_usage);
}

size_t TraceReader::get_current_memory_usage() const {
    if (memory_stats_dirty_) {
        update_memory_stats();
    }
    return static_cast<size_t>(cached_memory_stats_.current_usage);
}

TraceReader::MemoryStats TraceReader::get_memory_stats() const {
    if (memory_stats_dirty_) {
        update_memory_stats();
    }
    return cached_memory_stats_;
}

void TraceReader::update_memory_stats() {
    // Reset state
    cached_memory_stats_ = MemoryStats{};
    allocations_.clear();
    freed_pointers_.clear();
    current_usage_ = 0;
    
    // Pre-reserve to avoid rehashing during processing
    if (allocations_.bucket_count() < allocations_.size() * 2) {
        size_t memory_event_count = 0;
        for (const auto& event : events_) {
            if (event.type == EventType::TENSOR_ALLOC || event.type == EventType::TENSOR_FREE) {
                memory_event_count++;
            }
        }
        size_t estimated_allocs = std::max(memory_event_count / 2, size_t(1024));
        allocations_.reserve(estimated_allocs);
        freed_pointers_.reserve(estimated_allocs);
    }
    
    for (const auto& event : events_) {
        if (event.type == EventType::TENSOR_ALLOC) {
            cached_memory_stats_.total_allocations++;
            cached_memory_stats_.bytes_allocated += event.data.memory.size;
            
            if (cached_memory_stats_.first_alloc_time == 0) {
                cached_memory_stats_.first_alloc_time = event.timestamp_ns;
            }
            
            // Check for pointer reuse after free
            if (freed_pointers_.count(event.data.memory.ptr) > 0) {
                // This is expected in GGML - allocator reuses freed pointers
                freed_pointers_.erase(event.data.memory.ptr);
            }
            
            allocations_[event.data.memory.ptr] = event.data.memory.size;
            current_usage_ += event.data.memory.size;
            
            if (current_usage_ > cached_memory_stats_.peak_usage) {
                cached_memory_stats_.peak_usage = current_usage_;
            }
        } else if (event.type == EventType::TENSOR_FREE) {
            cached_memory_stats_.total_frees++;
            cached_memory_stats_.last_free_time = event.timestamp_ns;
            
            // Check for double-free
            if (freed_pointers_.count(event.data.memory.ptr) > 0) {
                std::cerr << "Warning: Double-free detected for pointer " << event.data.memory.ptr << "\n";
                continue;
            }
            
            auto it = allocations_.find(event.data.memory.ptr);
            if (it != allocations_.end()) {
                cached_memory_stats_.bytes_freed += it->second;
                current_usage_ -= it->second;
                allocations_.erase(it);
                freed_pointers_.insert(event.data.memory.ptr);
            } else {
                std::cerr << "Warning: Free without matching alloc for pointer " << event.data.memory.ptr << "\n";
            }
        }
    }
    
    // Calculate final values
    cached_memory_stats_.current_usage = current_usage_;
    for (const auto& pair : allocations_) {
        cached_memory_stats_.leaked_bytes += pair.second;
    }
    
    memory_stats_dirty_ = false;
}

} // namespace ggml_viz