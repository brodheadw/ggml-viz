// tests/test_ring_buffer.cpp
// Unit tests for lock-free SPSC ring buffer implementation

#include "ggml_hook.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <cassert>

using namespace ggml_viz;

// Test helper to create events
Event create_test_event(EventType type, uint64_t timestamp, uint32_t thread_id = 1) {
    Event e = {};
    e.type = type;
    e.timestamp_ns = timestamp;
    e.thread_id = thread_id;
    e.label = nullptr;
    return e;
}

// Test 1: Basic SPSC functionality 
bool test_basic_spsc() {
    std::cout << "Test 1: Basic SPSC functionality..." << std::flush;
    
    GGMLHook& hook = GGMLHook::instance();
    hook.start();
    
    // Producer: record some events
    for (int i = 0; i < 10; ++i) {
        Event e = create_test_event(EventType::OP_COMPUTE_BEGIN, i * 1000);
        hook.record_event(e);
    }
    
    // Consumer: consume events
    auto events = hook.consume_available_events();
    
    bool success = (events.size() == 10);
    for (size_t i = 0; i < events.size() && success; ++i) {
        success = (events[i].timestamp_ns == i * 1000);
    }
    
    hook.stop();
    hook.reset_stats();
    
    std::cout << (success ? " PASS" : " FAIL") << std::endl;
    return success;
}

// Test 2: Producer/consumer drift - flood producer, slow consumer
bool test_producer_consumer_drift() {
    std::cout << "Test 2: Producer/consumer drift..." << std::flush;
    
    constexpr int NUM_EVENTS = 1000;
    std::atomic<bool> stop_producer{false};
    std::atomic<int> events_produced{0};
    std::atomic<int> events_consumed{0};
    
    GGMLHook& hook = GGMLHook::instance();
    hook.start();
    
    // Producer thread - flood with events
    std::thread producer([&]() {
        int count = 0;
        while (!stop_producer.load() && count < NUM_EVENTS) {
            Event e = create_test_event(EventType::OP_COMPUTE_BEGIN, count);
            hook.record_event(e);
            events_produced.store(++count);
            // No delay - flood as fast as possible
        }
    });
    
    // Consumer thread - slow consumption
    std::thread consumer([&]() {
        int total_consumed = 0;
        while (total_consumed < NUM_EVENTS) {
            auto events = hook.consume_available_events();
            total_consumed += events.size();
            events_consumed.store(total_consumed);
            
            // Slow consumer - 1ms delay between reads
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Let producer run for a bit, then stop it
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop_producer.store(true);
    
    producer.join();
    consumer.join();
    
    bool success = (events_consumed.load() <= events_produced.load()) && 
                   (events_consumed.load() > 0);
    
    std::cout << " [Produced: " << events_produced.load() 
              << ", Consumed: " << events_consumed.load()
              << ", Dropped: " << hook.get_dropped_events() << "]";
    
    hook.stop();
    hook.reset_stats();
    
    std::cout << (success ? " PASS" : " FAIL") << std::endl;
    return success;
}

// Test 3: Wrap-around correctness with small buffer
bool test_wraparound() {
    std::cout << "Test 3: Wrap-around correctness..." << std::flush;
    
    GGMLHook& hook = GGMLHook::instance();
    hook.start();
    
    const size_t buffer_size = hook.get_buffer_size();
    const size_t test_events = buffer_size * 2; // Force multiple wraps
    
    // Fill buffer beyond its capacity to force wraparound
    for (size_t i = 0; i < test_events; ++i) {
        Event e = create_test_event(EventType::OP_COMPUTE_BEGIN, i);
        hook.record_event(e);
        
        // Consume periodically to prevent constant full buffer
        if (i % 100 == 0) {
            auto events = hook.consume_available_events();
            // Verify consumed events are sequential
            for (size_t j = 0; j < events.size(); ++j) {
                if (events[j].timestamp_ns != (i - events.size() + j + 1)) {
                    std::cout << " FAIL (sequence error)" << std::endl;
                    hook.stop();
                    hook.reset_stats();
                    return false;
                }
            }
        }
    }
    
    // Final consumption
    auto remaining = hook.consume_available_events();
    
    hook.stop();
    hook.reset_stats();
    
    std::cout << " PASS" << std::endl;
    return true;
}

// Test 4: Buffer full behavior (dropped events counter)
bool test_buffer_full() {
    std::cout << "Test 4: Buffer full behavior..." << std::flush;
    
    GGMLHook& hook = GGMLHook::instance();
    
    // Configure to disable file writing to prevent automatic consumption
    HookConfig config;
    config.write_to_file = false;
    hook.configure(config);
    hook.start();
    
    const size_t buffer_size = hook.get_buffer_size();
    
    // Fill buffer completely without consuming - need to overflow the "one empty slot" rule
    // Buffer size is 65536, so we need to fill 65535 slots + extra to trigger full condition
    for (size_t i = 0; i < buffer_size + 100; ++i) {
        Event e = create_test_event(EventType::OP_COMPUTE_BEGIN, i);
        hook.record_event(e);
        
        // Check dropped count periodically
        if (i > buffer_size - 10 && i % 10 == 0) {
            uint64_t current_dropped = hook.get_dropped_events();
            if (current_dropped > 0) {
                break; // Found drops, test successful
            }
        }
    }
    
    // Check that some events were dropped
    uint64_t dropped = hook.get_dropped_events();
    uint64_t final_write = hook.get_current_write_pos();
    uint64_t final_read = hook.get_current_read_pos();
    bool success = (dropped > 0);
    
    std::cout << " [Dropped: " << dropped << ", Write: " << final_write << ", Read: " << final_read << "]";
    
    hook.stop();
    hook.reset_stats();
    
    std::cout << (success ? " PASS" : " FAIL") << std::endl;
    return success;
}

// Test 5: Memory ordering stress test  
bool test_memory_ordering_stress() {
    std::cout << "Test 5: Memory ordering stress test..." << std::flush;
    
    constexpr int ITERATIONS = 5000;
    std::atomic<bool> stop_test{false};
    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};
    
    GGMLHook& hook = GGMLHook::instance();
    hook.start();
    
    // Single producer thread (true SPSC)
    std::thread producer([&]() {
        int produced = 0;
        while (produced < ITERATIONS && !stop_test.load()) {
            Event e = create_test_event(EventType::OP_COMPUTE_BEGIN, produced);
            hook.record_event(e);
            total_produced.store(++produced);
            
            // Brief yield to stress memory ordering
            if (produced % 10 == 0) {
                std::this_thread::yield();
            }
        }
    });
    
    // Single consumer thread
    std::thread consumer([&]() {
        int consumed = 0;
        while (consumed < ITERATIONS && !stop_test.load()) {
            auto events = hook.consume_available_events();
            consumed += events.size();
            total_consumed.store(consumed);
            
            // Brief yield to allow producer to work
            if (consumed % 50 == 0) {
                std::this_thread::yield();
            }
        }
    });
    
    // Wait for completion or timeout
    producer.join();
    consumer.join();
    
    // Get final counts
    int final_produced = total_produced.load();
    int final_consumed = total_consumed.load();
    
    // Final consumption of any remaining events
    auto remaining = hook.consume_available_events();
    final_consumed += remaining.size();
    
    bool success = (final_consumed == final_produced) && (final_produced > 0);
    
    std::cout << " [Produced: " << final_produced 
              << ", Consumed: " << final_consumed << "]";
    
    hook.stop();
    hook.reset_stats();
    
    std::cout << (success ? " PASS" : " FAIL") << std::endl;
    return success;
}

int main() {
    std::cout << "=== Lock-free SPSC Ring Buffer Tests ===" << std::endl;
    
    int passed = 0;
    int total = 5;
    
    if (test_basic_spsc()) passed++;
    if (test_producer_consumer_drift()) passed++;
    if (test_wraparound()) passed++;
    if (test_buffer_full()) passed++;
    if (test_memory_ordering_stress()) passed++;
    
    std::cout << std::endl;
    std::cout << "Results: " << passed << "/" << total << " tests passed";
    
    if (passed == total) {
        std::cout << " ✓" << std::endl;
        return 0;
    } else {
        std::cout << " ✗" << std::endl;
        return 1;
    }
}