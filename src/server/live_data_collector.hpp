// src/server/live_data_collector.hpp
#pragma once

#include "instrumentation/ggml_hook.hpp"
#include "utils/trace_reader.hpp"
#include <sys/stat.h>
#include <deque>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
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

// Follows a trace file written by another process (the hook runs in the traced program, not here).
class LiveDataCollector {
public:
    // Called with each batch of events read from the file; the events are only valid during the call.
    using EventCallback = std::function<void(const Event* events, size_t count)>;

private:
    std::atomic<bool> running_{false};
    std::thread collector_thread_;
    EventCallback event_callback_;
    std::string trace_file_;
    std::chrono::milliseconds poll_interval_{200};

public:
    LiveDataCollector() = default;
    ~LiveDataCollector() {
        stop();
    }

    void start(const std::string& trace_file, EventCallback callback) {
        if (running_.load()) {
            stop();
        }
        trace_file_ = trace_file;
        event_callback_ = callback;
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
        std::time_t last_mtime = 0;
        long long last_size = -1;
        size_t sent = 0;
        uint64_t first_ts = 0;

        while (running_.load()) {
            struct stat st;
            if (stat(trace_file_.c_str(), &st) == 0 &&
                (st.st_mtime != last_mtime || (long long)st.st_size != last_size)) {
                TraceReader reader(trace_file_);
                if (reader.is_valid()) {
                    const auto& events = reader.events();
                    // A smaller file or a different first event means it was recreated: start over.
                    if (events.size() < sent || (!events.empty() && sent > 0 && events.front().timestamp_ns != first_ts)) {
                        sent = 0;
                    }
                    if (events.size() > sent && event_callback_) {
                        event_callback_(events.data() + sent, events.size() - sent);
                    }
                    sent = events.size();
                    first_ts = events.empty() ? 0 : events.front().timestamp_ns;
                    last_mtime = st.st_mtime;
                    last_size = st.st_size;
                }
            }
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
        std::string trace_file;
        bool enable_websocket = true;
        bool enable_http_sse = true; // Server-Sent Events
    };

private:
    StreamConfig config_;
    LiveDataCollector collector_;
    std::atomic<bool> server_running_{false};
    std::thread server_thread_;
    
    // Recent events as JSON, each with a sequence number so every client keeps its own position
    std::deque<std::string> history_;
    uint64_t next_seq_ = 0;  // sequence number of the event after history_.back()
    std::mutex queue_mutex_;
    static constexpr size_t kMaxHistory = 10000;
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
        collector_.start(config_.trace_file, [this](const Event* events, size_t count) {
            handle_new_events(events, count);
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
    void handle_new_events(const Event* events, size_t count) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        for (size_t i = 0; i < count; i++) {
            history_.push_back(format_event_as_json(events[i]));
        }
        next_seq_ += count;
        while (history_.size() > kMaxHistory) {
            history_.pop_front();
        }
    }
    
    void server_loop() {
        printf("[LiveStreamServer] Starting HTTP server on %s:%d\n", config_.host.c_str(), config_.port);
        
        // Simple HTTP server implementation for Server-Sent Events (SSE)
        // This avoids external dependencies while providing live streaming
        start_http_server();
    }
    
    void start_http_server() {
#ifdef _WIN32
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
            printf("[LiveStreamServer] WSAStartup failed\n");
            return;
        }
#endif
        
        // Create socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            printf("[LiveStreamServer] Error creating socket\n");
            return;
        }
        
        // Allow socket reuse
        int opt = 1;
#ifdef _WIN32
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
        
        // Bind to address
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // live trace data is local-only
        address.sin_port = htons(config_.port);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            printf("[LiveStreamServer] Error binding to port %d\n", config_.port);
#ifdef _WIN32
            closesocket(server_fd);
#else
    #ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
#endif
            return;
        }
        
        // Listen for connections
        if (listen(server_fd, 10) < 0) {
            printf("[LiveStreamServer] Error listening on socket\n");
#ifdef _WIN32
            closesocket(server_fd);
#else
    #ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
#endif
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
        
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        printf("[LiveStreamServer] HTTP server stopped\n");
    }
    
    void handle_client(int client_fd) {
        connected_clients_.fetch_add(1);
        
        // Read HTTP request
        char buffer[4096];
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
#ifdef _WIN32
            closesocket(client_fd);
#else
    #ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
#endif
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
        
#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
        connected_clients_.fetch_sub(1);
    }
    
    void stream_events_to_client(int client_fd) {
        printf("[LiveStreamServer] Client connected for event streaming\n");
        
        uint64_t cursor = 0;  // next sequence number this client needs
        while (server_running_.load()) {
            std::vector<std::string> events_to_send;
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                uint64_t oldest = next_seq_ - history_.size();
                if (cursor < oldest) cursor = oldest;  // fell behind the retained history
                for (; cursor < next_seq_; cursor++) {
                    events_to_send.push_back(history_[cursor - oldest]);
                }
            }
            
            // Send events as SSE
            for (const auto& event_data : events_to_send) {
                std::string sse_message = "data: " + event_data + "\n\n";
                
#ifdef _WIN32
                int result = send(client_fd, sse_message.c_str(), (int)sse_message.length(), 0);
#else
                int result = send(client_fd, sse_message.c_str(), sse_message.length(), MSG_NOSIGNAL);
#endif
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
        char head[160];
        snprintf(head, sizeof(head), "{\"type\":\"%s\",\"timestamp\":%llu,\"thread_id\":%u,\"label\":\"",
            event_type_to_string(event.type), (unsigned long long)event.timestamp_ns, event.thread_id);
        std::string json = head;
        for (const char* c = event.label ? event.label : "unknown"; *c; c++) {
            unsigned char ch = static_cast<unsigned char>(*c);
            if (ch == '"' || ch == '\\') {
                json += '\\';
                json += *c;
            } else if (ch < 0x20) {
                char esc[8];
                snprintf(esc, sizeof(esc), "\\u%04x", ch);
                json += esc;
            } else {
                json += *c;
            }
        }
        json += "\"}";
        return json;
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
        return history_.size();
    }
};

// Constructor definitions outside class
inline LiveStreamServer::LiveStreamServer(const StreamConfig& config) : config_(config) {}

} // namespace ggml_viz