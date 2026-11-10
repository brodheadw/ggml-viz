// test_trace_reader.cpp - Test the TraceReader with our captured files
#include "utils/trace_reader.hpp" 
#include <cstdio>
#include <iostream>

// Re-allocating a live pointer must not double-count memory.
static bool check_realloc_accounting() {
    const char* path = "realloc_check.ggmlviz";
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    fwrite("GGMLVIZ1", 1, 8, f);
    uint32_t version = 1;
    fwrite(&version, sizeof(version), 1, f);
    ggml_viz::EventType types[] = {ggml_viz::EventType::TENSOR_ALLOC, ggml_viz::EventType::TENSOR_ALLOC,
                                   ggml_viz::EventType::TENSOR_FREE};
    for (uint64_t i = 0; i < 3; i++) {
        ggml_viz::Event e = {};
        e.type = types[i];
        e.timestamp_ns = i;
        e.data.memory.ptr = reinterpret_cast<const void*>(0x1000);
        e.data.memory.size = e.type == ggml_viz::EventType::TENSOR_ALLOC ? 100 : 0;
        uint8_t has_label = 0;
        fwrite(&e.type, sizeof(e.type), 1, f);
        fwrite(&e.timestamp_ns, sizeof(e.timestamp_ns), 1, f);
        fwrite(&e.thread_id, sizeof(e.thread_id), 1, f);
        fwrite(&e.data, sizeof(e.data), 1, f);
        fwrite(&has_label, 1, 1, f);
    }
    fclose(f);
    ggml_viz::TraceReader reader(path);
    bool ok = reader.is_valid() && reader.get_peak_memory_usage() == 100 && reader.get_current_memory_usage() == 0;
    std::remove(path);
    return ok;
}

int main(int argc, const char* const argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <trace_file.ggmlviz>" << std::endl;
        return 1;
    }
    
    if (!check_realloc_accounting()) {
        std::cout << "❌ Re-alloc of a live pointer double-counted memory" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::cout << "=== Testing TraceReader with: " << filename << " ===" << std::endl;
    
    try {
        ggml_viz::TraceReader reader(filename);
        
        if (!reader.is_valid()) {
            std::cout << "❌ Failed to load trace file" << std::endl;
            return 1;
        }
        
        std::cout << "✅ Trace file loaded successfully!" << std::endl;
        std::cout << "📊 Event count: " << reader.event_count() << std::endl;
        std::cout << "⏱️  Total duration: " << reader.get_total_duration_ns() << " ns" << std::endl;
        
        auto graph_events = reader.get_graph_events();
        std::cout << "📈 Graph events: " << graph_events.size() << std::endl;
        
        auto op_timings = reader.get_op_timings();
        std::cout << "⚡ Operation timings: " << op_timings.size() << std::endl;
        
        if (!op_timings.empty()) {
            std::cout << "\n🔥 Top 5 slowest operations:" << std::endl;
            for (size_t i = 0; i < std::min(size_t(5), op_timings.size()); i++) {
                const auto& timing = op_timings[i];
                double duration_ms = timing.duration_ns / 1000000.0;
                std::cout << "  " << (i+1) << ". " << timing.name 
                         << " - " << duration_ms << " ms" << std::endl;
            }
        }
        
        std::cout << "\n✅ TraceReader test completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}