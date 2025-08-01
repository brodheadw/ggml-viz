// test_trace_reader.cpp - Test the TraceReader with our captured files
#include "utils/trace_reader.hpp" 
#include <iostream>

int main(int argc, const char* const argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <trace_file.ggmlviz>" << std::endl;
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