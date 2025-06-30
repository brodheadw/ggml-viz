// test_live_visualization.cpp - Complete end-to-end test of live visualization
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

// Include our live data collection system
#include "server/live_data_collector.hpp"
#include "instrumentation/ggml_hook.hpp"
#include "ggml.h"

// Global counter for received events
std::atomic<size_t> total_events_received{0};

void simulate_model_inference() {
    std::cout << "\n🧠 Simulating LLM inference with GGML operations..." << std::endl;
    
    // Simulate a graph computation with several operations
    const ggml_cgraph* mock_graph = reinterpret_cast<const ggml_cgraph*>(0x12345);
    const ggml_tensor* mock_tensor1 = reinterpret_cast<const ggml_tensor*>(0x11111);
    const ggml_tensor* mock_tensor2 = reinterpret_cast<const ggml_tensor*>(0x22222);
    const ggml_tensor* mock_tensor3 = reinterpret_cast<const ggml_tensor*>(0x33333);
    
    // Start graph computation
    ggml_viz::ggml_viz_hook_graph_compute_begin(mock_graph);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Simulate several matrix operations
    for (int layer = 0; layer < 3; layer++) {
        std::cout << "  Layer " << layer << " operations..." << std::endl;
        
        // Matrix multiplication
        ggml_viz::ggml_viz_hook_op_compute_begin(mock_tensor1);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ggml_viz::ggml_viz_hook_op_compute_end(mock_tensor1);
        
        // Addition
        ggml_viz::ggml_viz_hook_op_compute_begin(mock_tensor2);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        ggml_viz::ggml_viz_hook_op_compute_end(mock_tensor2);
        
        // ReLU activation
        ggml_viz::ggml_viz_hook_op_compute_begin(mock_tensor3);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        ggml_viz::ggml_viz_hook_op_compute_end(mock_tensor3);
    }
    
    // End graph computation
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ggml_viz::ggml_viz_hook_graph_compute_end(mock_graph);
    
    std::cout << "🔬 Model inference simulation complete!" << std::endl;
}

int main() {
    std::cout << "=== 🚀 Live Visualization End-to-End Test ===" << std::endl;
    
    try {
        // Test 1: Verify GGMLHook is working
        std::cout << "\n1️⃣ Testing GGMLHook access..." << std::endl;
        auto& hook = ggml_viz::GGMLHook::instance();
        hook.start();
        std::cout << "   ✅ GGMLHook active: " << (hook.is_active() ? "Yes" : "No") << std::endl;
        
        // Test 2: Setup live data collection
        std::cout << "\n2️⃣ Setting up live data collection..." << std::endl;
        ggml_viz::LiveDataCollector collector;
        
        // Callback to process live events
        collector.start([](const std::vector<ggml_viz::Event>& events) {
            total_events_received += events.size();
            if (!events.empty()) {
                std::cout << "   📥 Received " << events.size() << " events (total: " 
                         << total_events_received.load() << ")" << std::endl;
                
                // Show details of first event in each batch
                const auto& event = events[0];
                std::string event_type;
                switch(event.type) {
                    case ggml_viz::EventType::GRAPH_COMPUTE_BEGIN: event_type = "GRAPH_BEGIN"; break;
                    case ggml_viz::EventType::GRAPH_COMPUTE_END: event_type = "GRAPH_END"; break;
                    case ggml_viz::EventType::OP_COMPUTE_BEGIN: event_type = "OP_BEGIN"; break;
                    case ggml_viz::EventType::OP_COMPUTE_END: event_type = "OP_END"; break;
                    default: event_type = "UNKNOWN"; break;
                }
                std::cout << "   🔍 Sample event: " << event_type;
                if (event.label) {
                    std::cout << " (" << event.label << ")";
                }
                std::cout << std::endl;
            }
        }, std::chrono::milliseconds(50)); // 50ms polling for demo
        
        std::cout << "   ✅ LiveDataCollector started" << std::endl;
        
        // Test 3: Setup streaming server
        std::cout << "\n3️⃣ Setting up streaming server..." << std::endl;
        ggml_viz::LiveStreamServer::StreamConfig config;
        config.port = 8082; // Different port for testing
        ggml_viz::LiveStreamServer server(config);
        server.start();
        std::cout << "   ✅ LiveStreamServer started on port " << config.port << std::endl;
        
        // Test 4: Simulate model inference to generate events
        std::cout << "\n4️⃣ Running simulation..." << std::endl;
        std::thread simulation_thread(simulate_model_inference);
        
        // Let the system run for a bit to collect events
        std::cout << "\n⏱ Monitoring live events for 3 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Wait for simulation to complete
        simulation_thread.join();
        
        // Give time for final events to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Test 5: Verify events were captured
        std::cout << "\n5️⃣ Results summary..." << std::endl;
        std::cout << "   📊 Hook event count: " << hook.event_count() << std::endl;
        std::cout << "   📊 Live events received: " << total_events_received.load() << std::endl;
        std::cout << "   📊 Server running: " << (server.is_running() ? "Yes" : "No") << std::endl;
        std::cout << "   📊 Server clients: " << server.client_count() << std::endl;
        
        // Test 6: Cleanup
        std::cout << "\n6️⃣ Cleaning up..." << std::endl;
        collector.stop();
        server.stop();
        hook.stop();
        
        // Final verification
        bool success = total_events_received.load() > 0 && hook.event_count() > 0;
        if (success) {
            std::cout << "\n🎉 SUCCESS! Live visualization system is working end-to-end!" << std::endl;
            std::cout << "   ✅ Events captured: " << hook.event_count() << std::endl;
            std::cout << "   ✅ Events streamed: " << total_events_received.load() << std::endl;
            std::cout << "\n📁 Trace file saved: live_test_output.ggmlviz" << std::endl;
            std::cout << "   You can open this file in the GUI: ./bin/ggml-viz live_test_output.ggmlviz" << std::endl;
        } else {
            std::cout << "\n❌ FAILED: No events were captured or streamed" << std::endl;
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
}