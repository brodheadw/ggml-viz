// test_live_system.cpp - Test the live visualization system
#include <iostream>
#include <chrono>
#include <thread>

// Test if we can include our headers without errors
#include "instrumentation/ggml_hook.hpp"
#include "server/live_data_collector.hpp"

int main() {
    std::cout << "=== Live System Integration Test ===" << std::endl;
    
    try {
        // Test 1: GGMLHook instance access
        std::cout << "1. Testing GGMLHook access..." << std::endl;
        auto& hook = ggml_viz::GGMLHook::instance();
        std::cout << "   ✅ GGMLHook instance accessible" << std::endl;
        std::cout << "   📊 Active: " << (hook.is_active() ? "Yes" : "No") << std::endl;
        std::cout << "   📈 Event count: " << hook.event_count() << std::endl;
        
        // Test 2: Live Data Collector
        std::cout << "\n2. Testing LiveDataCollector..." << std::endl;
        ggml_viz::LiveDataCollector collector;
        std::cout << "   ✅ LiveDataCollector created" << std::endl;
        std::cout << "   📊 Running: " << (collector.is_running() ? "Yes" : "No") << std::endl;
        
        // Test 3: Start live collection with callback
        std::cout << "\n3. Testing live event collection..." << std::endl;
        size_t events_received = 0;
        
        collector.start([&events_received](const std::vector<ggml_viz::Event>& events) {
            events_received += events.size();
            if (!events.empty()) {
                std::cout << "   📥 Received " << events.size() << " events (total: " 
                         << events_received << ")" << std::endl;
            }
        }, std::chrono::milliseconds(50)); // 50ms polling for test
        
        std::cout << "   ✅ Live collector started" << std::endl;
        std::cout << "   📊 Running: " << (collector.is_running() ? "Yes" : "No") << std::endl;
        
        // Test 4: Let it run for a short time
        std::cout << "\n4. Monitoring for 2 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Test 5: Stop collector
        std::cout << "\n5. Stopping collector..." << std::endl;
        collector.stop();
        std::cout << "   ✅ Collector stopped" << std::endl;
        std::cout << "   📊 Total events received: " << events_received << std::endl;
        
        // Test 6: Live Stream Server (basic test)
        std::cout << "\n6. Testing LiveStreamServer..." << std::endl;
        ggml_viz::LiveStreamServer::StreamConfig config;
        config.port = 8081; // Use different port for test
        
        ggml_viz::LiveStreamServer server(config);
        std::cout << "   ✅ LiveStreamServer created" << std::endl;
        
        server.start();
        std::cout << "   ✅ Server started on port " << config.port << std::endl;
        std::cout << "   📊 Running: " << (server.is_running() ? "Yes" : "No") << std::endl;
        std::cout << "   👥 Clients: " << server.client_count() << std::endl;
        
        // Let server run briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        server.stop();
        std::cout << "   ✅ Server stopped" << std::endl;
        
        std::cout << "\n🎉 All tests completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "❌ Unknown exception occurred" << std::endl;
        return 1;
    }
}