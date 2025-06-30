// src/server/live_data_collector.hpp
#pragma once

#include "instrumentation/ggml_hook.hpp"
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <queue>

namespace ggml_viz {

class LiveDataCollector {
public:
    using EventCallback = std::function<void(const std::vector<Event>&)>;

private:
    std::atomic<bool> running_{false};
    std::thread collector_thread_;
    EventCallback event_callback_;
    std::chrono::milliseconds poll_interval_{10}; // 10ms polling
    
    // Buffer for accumulating events before sending
    std::vector<Event> event_buffer_;
    std::mutex buffer_mutex_;
    
public:
    LiveDataCollector() = default;
    ~LiveDataCollector() {
        stop();
    }

    // Start live collection with callback for new events
    void start(EventCallback callback, std::chrono::milliseconds poll_interval = std::chrono::milliseconds(10)) {
        if (running_.load()) {
            stop();
        }
        
        event_callback_ = callback;
        poll_interval_ = poll_interval;
        running_ = true;
        
        collector_thread_ = std::thread(&LiveDataCollector::collector_loop, this);
    }
    
    void stop() {
        if (running_.load()) {
            running_ = false;
            if (collector_thread_.joinable()) {
                collector_thread_.join();
            }
        }
    }
    
    bool is_running() const {
        return running_.load();
    }

private:
    void collector_loop() {
        auto& hook = GGMLHook::instance();
        
        while (running_.load()) {
            // Check if hook is active
            if (!hook.is_active()) {
                std::this_thread::sleep_for(poll_interval_);
                continue;
            }
            
            // Consume available events from the ring buffer
            auto new_events = hook.consume_available_events();
            
            if (!new_events.empty()) {
                // Send events to callback
                if (event_callback_) {
                    event_callback_(new_events);
                }
            }
            
            // Sleep for polling interval
            std::this_thread::sleep_for(poll_interval_);
        }
    }
};

// WebSocket/HTTP streaming server for live events
class LiveStreamServer {
public:
    struct StreamConfig {
        int port = 8080;
        std::string host = "localhost";
        bool enable_websocket = true;
        bool enable_http_sse = true; // Server-Sent Events
    };

private:
    StreamConfig config_;
    LiveDataCollector collector_;
    std::atomic<bool> server_running_{false};
    std::thread server_thread_;
    
    // Simple event queue for streaming
    std::queue<Event> event_queue_;
    std::mutex queue_mutex_;
    std::atomic<size_t> connected_clients_{0};

public:
    LiveStreamServer() = default;
    explicit LiveStreamServer(const StreamConfig& config);
    
    ~LiveStreamServer() {
        stop();
    }
    
    void start() {
        if (server_running_.load()) return;
        
        server_running_ = true;
        
        // Start the data collector with our event handler
        collector_.start([this](const std::vector<Event>& events) {
            handle_new_events(events);
        });
        
        // Start the streaming server
        server_thread_ = std::thread(&LiveStreamServer::server_loop, this);
        
        printf("[LiveStreamServer] Started on %s:%d\n", config_.host.c_str(), config_.port);
    }
    
    void stop() {
        if (server_running_.load()) {
            server_running_ = false;
            collector_.stop();
            
            if (server_thread_.joinable()) {
                server_thread_.join();
            }
            
            printf("[LiveStreamServer] Stopped\n");
        }
    }
    
    bool is_running() const {
        return server_running_.load();
    }
    
    size_t client_count() const {
        return connected_clients_.load();
    }

private:
    void handle_new_events(const std::vector<Event>& events) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        // Add events to streaming queue
        for (const auto& event : events) {
            event_queue_.push(event);
        }
        
        // Limit queue size to prevent memory issues
        while (event_queue_.size() > 10000) {
            event_queue_.pop();
        }
        
        // In a real implementation, this would trigger WebSocket sends
        if (connected_clients_.load() > 0) {
            printf("[LiveStreamServer] Queued %zu events for %zu clients\n", 
                   events.size(), connected_clients_.load());
        }
    }
    
    void server_loop() {
        // This is a placeholder for the actual server implementation
        // In a real implementation, you would use a WebSocket library like:
        // - libwebsockets
        // - websocketpp
        // - uWebSockets
        // Or implement HTTP Server-Sent Events
        
        printf("[LiveStreamServer] Server loop started (placeholder implementation)\n");
        
        while (server_running_.load()) {
            // Simulate client connection handling
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // In real implementation: 
            // - Accept WebSocket connections
            // - Send queued events to connected clients
            // - Handle client disconnections
        }
    }
};

// Constructor definitions outside class
inline LiveStreamServer::LiveStreamServer(const StreamConfig& config) : config_(config) {}

} // namespace ggml_viz