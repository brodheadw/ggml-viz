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
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif
#include <cstring>
#include <cstdio>

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
        printf("[LiveStreamServer] Starting HTTP server on %s:%d\n", config_.host.c_str(), config_.port);
        
        // Simple HTTP server implementation for Server-Sent Events (SSE)
        // This avoids external dependencies while providing live streaming
        start_http_server();
    }
    
    void start_http_server() {
        // Create socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            printf("[LiveStreamServer] Error creating socket\n");
            return;
        }
        
        // Allow socket reuse
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Bind to address
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(config_.port);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            printf("[LiveStreamServer] Error binding to port %d\n", config_.port);
            close(server_fd);
            return;
        }
        
        // Listen for connections
        if (listen(server_fd, 10) < 0) {
            printf("[LiveStreamServer] Error listening on socket\n");
            close(server_fd);
            return;
        }
        
        printf("[LiveStreamServer] HTTP server listening on port %d\n", config_.port);
        
        while (server_running_.load()) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd, &read_fds);
            
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            int activity = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);
            
            if (activity > 0 && FD_ISSET(server_fd, &read_fds)) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd >= 0) {
                    // Handle client in separate thread
                    std::thread(&LiveStreamServer::handle_client, this, client_fd).detach();
                }
            }
        }
        
        close(server_fd);
        printf("[LiveStreamServer] HTTP server stopped\n");
    }
    
    void handle_client(int client_fd) {
        connected_clients_.fetch_add(1);
        
        // Read HTTP request
        char buffer[4096];
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            close(client_fd);
            connected_clients_.fetch_sub(1);
            return;
        }
        buffer[bytes_read] = '\0';
        
        // Parse request path
        std::string request(buffer);
        bool is_events_endpoint = request.find("GET /events") == 0;
        bool is_status_endpoint = request.find("GET /status") == 0;
        
        if (is_events_endpoint) {
            // Send SSE headers
            std::string headers = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/event-stream\r\n"
                "Cache-Control: no-cache\r\n"
                "Connection: keep-alive\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "\r\n";
            send(client_fd, headers.c_str(), headers.length(), 0);
            
            // Stream events
            stream_events_to_client(client_fd);
        } else if (is_status_endpoint) {
            // Send status JSON
            char status_json[512];
            snprintf(status_json, sizeof(status_json),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "\r\n"
                "{\"status\":\"running\",\"clients\":%zu,\"events_queued\":%zu}\r\n",
                connected_clients_.load(), get_queue_size());
            send(client_fd, status_json, strlen(status_json), 0);
        } else {
            // Send simple HTML dashboard
            std::string html = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<!DOCTYPE html><html><head><title>GGML Visualizer Live</title></head>"
                "<body><h1>GGML Visualizer Live Stream</h1>"
                "<p>Status: <span id='status'>Connecting...</span></p>"
                "<p>Events: <span id='events'>0</span></p>"
                "<div id='log'></div>"
                "<script>"
                "const eventSource = new EventSource('/events');"
                "let eventCount = 0;"
                "eventSource.onmessage = function(e) {"
                "  eventCount++;"
                "  document.getElementById('events').textContent = eventCount;"
                "  document.getElementById('status').textContent = 'Connected';"
                "  const log = document.getElementById('log');"
                "  const div = document.createElement('div');"
                "  div.textContent = new Date().toLocaleTimeString() + ': ' + e.data;"
                "  log.appendChild(div);"
                "  if (log.children.length > 100) log.removeChild(log.firstChild);"
                "};"
                "eventSource.onerror = function() {"
                "  document.getElementById('status').textContent = 'Disconnected';"
                "};"
                "</script></body></html>";
            send(client_fd, html.c_str(), html.length(), 0);
        }
        
        close(client_fd);
        connected_clients_.fetch_sub(1);
    }
    
    void stream_events_to_client(int client_fd) {
        printf("[LiveStreamServer] Client connected for event streaming\n");
        
        while (server_running_.load()) {
            // Get events from queue
            std::vector<Event> events_to_send;
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                while (!event_queue_.empty() && events_to_send.size() < 10) {
                    events_to_send.push_back(event_queue_.front());
                    event_queue_.pop();
                }
            }
            
            // Send events as SSE
            for (const auto& event : events_to_send) {
                std::string event_data = format_event_as_json(event);
                std::string sse_message = "data: " + event_data + "\n\n";
                
                int result = send(client_fd, sse_message.c_str(), sse_message.length(), MSG_NOSIGNAL);
                if (result <= 0) {
                    printf("[LiveStreamServer] Client disconnected\n");
                    return;
                }
            }
            
            // Sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    std::string format_event_as_json(const Event& event) {
        char json[512];
        snprintf(json, sizeof(json),
            "{\"type\":\"%s\",\"timestamp\":%llu,\"thread_id\":%u,\"label\":\"%s\"}",
            event_type_to_string(event.type),
            (unsigned long long)event.timestamp_ns,
            event.thread_id,
            event.label ? event.label : "unknown");
        return std::string(json);
    }
    
    const char* event_type_to_string(EventType type) {
        switch (type) {
            case EventType::GRAPH_COMPUTE_BEGIN: return "graph_begin";
            case EventType::GRAPH_COMPUTE_END: return "graph_end";
            case EventType::OP_COMPUTE_BEGIN: return "op_begin";
            case EventType::OP_COMPUTE_END: return "op_end";
            case EventType::TENSOR_ALLOC: return "tensor_alloc";
            case EventType::TENSOR_FREE: return "tensor_free";
            case EventType::BARRIER_WAIT: return "barrier_wait";
            case EventType::THREAD_BEGIN: return "thread_begin";
            case EventType::THREAD_FREE: return "thread_free";
            default: return "unknown";
        }
    }
    
    size_t get_queue_size() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queue_mutex_));
        return event_queue_.size();
    }
};

// Constructor definitions outside class
inline LiveStreamServer::LiveStreamServer(const StreamConfig& config) : config_(config) {}

} // namespace ggml_viz