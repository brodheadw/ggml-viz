// src/server/data_collector.cpp
#include "data_collector.hpp"
#include "live_data_collector.hpp"
#include "instrumentation/ggml_hook.hpp"
#include "ggml.h"
#include <iostream>
#include <cstring>

namespace ggml_viz {

// Initialize static instance
DataCollector* DataCollector::instance_ = nullptr;

// Enhanced DataCollector implementation that can work with both
// offline file writing and live streaming
class EnhancedDataCollector : public DataCollector {
private:
    std::unique_ptr<LiveStreamServer> live_server_;
    bool live_mode_enabled_ = false;
    
public:
    void enable_live_streaming(int port = 8080) {
        live_mode_enabled_ = true;
        
        LiveStreamServer::StreamConfig config;
        config.port = port;
        config.host = "localhost";
        
        live_server_ = std::make_unique<LiveStreamServer>(config);
        live_server_->start();
        
        std::cout << "[DataCollector] Live streaming enabled on port " << port << std::endl;
    }
    
    void disable_live_streaming() {
        if (live_server_) {
            live_server_->stop();
            live_server_.reset();
        }
        live_mode_enabled_ = false;
        std::cout << "[DataCollector] Live streaming disabled" << std::endl;
    }
    
    bool is_live_mode() const {
        return live_mode_enabled_ && live_server_ && live_server_->is_running();
    }
    
    size_t connected_clients() const {
        return live_server_ ? live_server_->client_count() : 0;
    }
    
    // Override record_event to support both file and live modes
    void record_event(const TraceEvent& event) {
        // Call parent implementation for file-based recording
        DataCollector::record_event(event);
        
        // In live mode, events are automatically consumed by LiveDataCollector
        // from the GGMLHook ring buffer, so no additional action needed here
    }
};

// Factory function to create enhanced data collector
std::unique_ptr<DataCollector> create_data_collector(bool enable_live = false, int port = 8080) {
    auto collector = std::make_unique<EnhancedDataCollector>();
    
    if (enable_live) {
        collector->enable_live_streaming(port);
    }
    
    return collector;
}

// Convenience functions for common usage patterns
namespace collector_utils {
    
    void start_offline_collection(const std::string& filename) {
        auto* collector = DataCollector::getInstance();
        collector->enable(filename);
        std::cout << "[DataCollector] Started offline collection to: " << filename << std::endl;
    }
    
    void start_live_collection(int port = 8080) {
        auto collector = create_data_collector(true, port);
        std::cout << "[DataCollector] Started live collection on port: " << port << std::endl;
    }
    
    void start_hybrid_collection(const std::string& filename, int port = 8080) {
        auto collector = create_data_collector(true, port);
        collector->enable(filename);
        std::cout << "[DataCollector] Started hybrid collection - file: " << filename 
                  << ", live port: " << port << std::endl;
    }
    
    // Utility functions for data collection
    void log_collection_stats() {
        auto* collector = DataCollector::getInstance();
        std::cout << "[DataCollector] Current event count: " << collector->event_count() << std::endl;
    }
}

} // namespace ggml_viz