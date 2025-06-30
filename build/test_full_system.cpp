// test_full_system.cpp - Complete test with proper file output and terminal display
#include <iostream>
#include <thread>
#include <chrono>

#include "instrumentation/ggml_hook.hpp"
#include "server/data_collector.hpp" 
#include "utils/trace_reader.hpp"
#include "ggml.h"

int main() {
    std::cout << "=== 🔬 Full System Test: Hook → File → Reader ===" << std::endl;
    
    const std::string trace_file = "full_system_test.ggmlviz";
    
    try {
        // Test 1: Initialize GGMLHook for file output
        std::cout << "\n1️⃣ Starting GGMLHook..." << std::endl;
        auto& hook = ggml_viz::GGMLHook::instance();
        hook.start();
        std::cout << "   ✅ GGMLHook active: " << (hook.is_active() ? "Yes" : "No") << std::endl;
        
        // Test 2: Setup DataCollector for traditional file output
        std::cout << "\n2️⃣ Setting up DataCollector..." << std::endl;
        auto* collector = ggml_viz::DataCollector::getInstance();
        collector->enable(trace_file);
        std::cout << "   ✅ DataCollector enabled for file: " << trace_file << std::endl;
        
        // Test 3: Generate events using both systems
        std::cout << "\n3️⃣ Generating GGML events..." << std::endl;
        
        // Create mock GGML structures
        const ggml_cgraph* mock_graph = reinterpret_cast<const ggml_cgraph*>(0x12345);
        const ggml_tensor* mock_tensor1 = reinterpret_cast<const ggml_tensor*>(0x11111);
        const ggml_tensor* mock_tensor2 = reinterpret_cast<const ggml_tensor*>(0x22222);
        
        for (int run = 0; run < 5; run++) {
            std::cout << "   Run " << (run + 1) << "/5..." << std::endl;
            
            // Use GGMLHook functions (like injection would)
            ggml_viz::ggml_viz_hook_graph_compute_begin(mock_graph);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            
            for (int op = 0; op < 10; op++) {
                ggml_viz::ggml_viz_hook_op_compute_begin(mock_tensor1);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                ggml_viz::ggml_viz_hook_op_compute_end(mock_tensor1);
                
                ggml_viz::ggml_viz_hook_op_compute_begin(mock_tensor2);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                ggml_viz::ggml_viz_hook_op_compute_end(mock_tensor2);
            }
            
            ggml_viz::ggml_viz_hook_graph_compute_end(mock_graph);
            
            // Also create TraceEvents for DataCollector
            ggml_viz::TraceEvent graph_begin{
                ggml_viz::EventType::GRAPH_COMPUTE_BEGIN,
                static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
                std::this_thread::get_id(),
                mock_graph
            };
            collector->record_event(graph_begin);
            
            ggml_viz::TraceEvent op_begin{
                ggml_viz::EventType::OP_COMPUTE_BEGIN,
                static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
                std::this_thread::get_id(),
                nullptr,
                GGML_OP_MUL_MAT,
                "test_mul_mat"
            };
            collector->record_event(op_begin);
            
            ggml_viz::TraceEvent op_end{
                ggml_viz::EventType::OP_COMPUTE_END,
                static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
                std::this_thread::get_id(),
                nullptr,
                GGML_OP_MUL_MAT,
                "test_mul_mat"
            };
            collector->record_event(op_end);
            
            ggml_viz::TraceEvent graph_end{
                ggml_viz::EventType::GRAPH_COMPUTE_END,
                static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
                std::this_thread::get_id(),
                mock_graph
            };
            collector->record_event(graph_end);
        }
        
        std::cout << "   📊 GGMLHook events: " << hook.event_count() << std::endl;
        std::cout << "   📊 DataCollector events: " << collector->event_count() << std::endl;
        
        // Test 4: Force flush both systems
        std::cout << "\n4️⃣ Flushing data to files..." << std::endl;
        
        // Flush DataCollector 
        collector->flush();
        std::cout << "   ✅ DataCollector flushed to: " << trace_file << std::endl;
        
        // Force GGMLHook flush (it normally flushes every 4096 events)
        hook.stop(); // This should flush remaining events
        std::cout << "   ✅ GGMLHook stopped and flushed" << std::endl;
        
        // Test 5: Read back the file and verify
        std::cout << "\n5️⃣ Reading back trace file..." << std::endl;
        ggml_viz::TraceReader reader(trace_file);
        
        if (!reader.is_valid()) {
            std::cout << "   ❌ Failed to read trace file" << std::endl;
            return 1;
        }
        
        std::cout << "   ✅ Trace file loaded successfully" << std::endl;
        std::cout << "   📊 Events in file: " << reader.event_count() << std::endl;
        std::cout << "   ⏱ Total duration: " << (reader.get_total_duration_ns() / 1e6) << " ms" << std::endl;
        
        // Show some events
        const auto& events = reader.events();
        std::cout << "\n📋 Sample events:" << std::endl;
        for (size_t i = 0; i < std::min<size_t>(5, events.size()); i++) {
            const auto& event = events[i];
            std::string event_type;
            switch(event.type) {
                case ggml_viz::EventType::GRAPH_COMPUTE_BEGIN: event_type = "GRAPH_BEGIN"; break;
                case ggml_viz::EventType::GRAPH_COMPUTE_END: event_type = "GRAPH_END"; break;
                case ggml_viz::EventType::OP_COMPUTE_BEGIN: event_type = "OP_BEGIN"; break;
                case ggml_viz::EventType::OP_COMPUTE_END: event_type = "OP_END"; break;
                default: event_type = "UNKNOWN"; break;
            }
            
            std::cout << "   [" << i << "] " << event_type;
            if (event.label) {
                std::cout << " (" << event.label << ")";
            }
            std::cout << " @ " << (event.timestamp_ns / 1e6) << "ms" << std::endl;
        }
        
        // Get operation timings
        auto op_timings = reader.get_op_timings();
        if (!op_timings.empty()) {
            std::cout << "\n⏱ Operation timings:" << std::endl;
            for (size_t i = 0; i < std::min<size_t>(3, op_timings.size()); i++) {
                const auto& timing = op_timings[i];
                std::cout << "   " << timing.name << ": " 
                         << (timing.duration_ns / 1e6) << " ms" << std::endl;
            }
        }
        
        // Test 6: Final verification
        std::cout << "\n6️⃣ Final verification..." << std::endl;
        bool success = reader.event_count() > 0;
        if (success) {
            std::cout << "🎉 SUCCESS! Full system working end-to-end!" << std::endl;
            std::cout << "   📁 Trace file: " << trace_file << std::endl;
            std::cout << "   🖥 Open in GUI: ./bin/ggml-viz " << trace_file << std::endl;
        } else {
            std::cout << "❌ FAILED: No events in trace file" << std::endl;
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
}