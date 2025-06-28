// tests/test_trace_reader.cpp
#include "utils/trace_reader.hpp"
#include "instrumentation/ggml_hook.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <thread>

using namespace ggml_viz;

void create_test_trace(const std::string& filename) {
    std::cout << "Creating test trace file: " << filename << "\n";

    FILE* file = fopen(filename.c_str(), "wb");
    assert(file != nullptr);

    const char magic[] = "GGMLVIZ1";
    fwrite(magic, 1, 8, file);
    uint32_t version = 1;
    fwrite(&version, sizeof(version), 1, file);

    auto write_event = [&](EventType type, uint64_t timestamp, uint32_t thread_id,
                           const char* label, uint32_t op_type = 0) {
        Event event = {};
        event.type = type;
        event.timestamp_ns = timestamp;
        event.thread_id = thread_id;
        
        if (type == EventType::OP_COMPUTE_BEGIN || type == EventType::OP_COMPUTE_END) {
            event.data.op.tensor_ptr = reinterpret_cast<void*>(0x1000 + op_type);
            event.data.op.op_type = op_type;
            event.data.op.op_size = 1024 * (op_type + 1);
        } else {
            event.data.graph.graph_ptr = reinterpret_cast<void*>(0x2000);
            event.data.graph.n_nodes = 10;
            event.data.graph.n_threads = 4;
        }
        
        // Write event
        fwrite(&event.type, sizeof(event.type), 1, file);
        fwrite(&event.timestamp_ns, sizeof(event.timestamp_ns), 1, file);
        fwrite(&event.thread_id, sizeof(event.thread_id), 1, file);
        fwrite(&event.data, sizeof(event.data), 1, file);
        
        // Write label
        uint8_t has_label = (label != nullptr) ? 1 : 0;
        fwrite(&has_label, 1, 1, file);
        if (has_label) {
            uint32_t label_len = strlen(label);
            fwrite(&label_len, sizeof(label_len), 1, file);
            fwrite(label, 1, label_len, file);
        }
    };
    
    // Simulate a computation sequence
    uint64_t base_time = 1000000000; // 1 second in nanoseconds
    
    // Graph computation begin
    write_event(EventType::GRAPH_COMPUTE_BEGIN, base_time, 1, nullptr);
    
    // Operation sequence: MUL_MAT
    write_event(EventType::OP_COMPUTE_BEGIN, base_time + 1000, 1, "matmul_AB", 26);
    write_event(EventType::OP_COMPUTE_END, base_time + 50000, 1, "matmul_AB", 26);
    
    // Operation sequence: ADD
    write_event(EventType::OP_COMPUTE_BEGIN, base_time + 51000, 1, "add_bias", 2);
    write_event(EventType::OP_COMPUTE_END, base_time + 55000, 1, "add_bias", 2);
    
    // Operation sequence: RMS_NORM
    write_event(EventType::OP_COMPUTE_BEGIN, base_time + 60000, 2, "rms_norm", 23);
    write_event(EventType::OP_COMPUTE_END, base_time + 70000, 2, "rms_norm", 23);
    
    // Graph computation end
    write_event(EventType::GRAPH_COMPUTE_END, base_time + 100000, 1, nullptr);
    
    fclose(file);
    std::cout << "Test trace file created successfully\n";
}

// Test basic file loading
void test_basic_loading() {
    std::cout << "\n=== Test: Basic Loading ===\n";
    
    const char* filename = "test_basic.ggmlviz";
    create_test_trace(filename);
    
    TraceReader reader(filename);
    assert(reader.is_valid());
    assert(reader.event_count() == 8); // 8 events written
    
    std::cout << "✓ Basic loading test passed\n";
}

// Test invalid file handling
void test_invalid_file() {
    std::cout << "\n=== Test: Invalid File ===\n";
    
    TraceReader reader("nonexistent_file.ggmlviz");
    assert(!reader.is_valid());
    assert(reader.event_count() == 0);
    
    std::cout << "✓ Invalid file test passed\n";
}

// Test event filtering
void test_event_filtering() {
    std::cout << "\n=== Test: Event Filtering ===\n";
    
    const char* filename = "test_filter.ggmlviz";
    create_test_trace(filename);
    
    TraceReader reader(filename);
    assert(reader.is_valid());
    
    // Test graph events
    auto graph_events = reader.get_graph_events();
    assert(graph_events.size() == 2); // BEGIN and END
    assert(graph_events[0]->type == EventType::GRAPH_COMPUTE_BEGIN);
    assert(graph_events[1]->type == EventType::GRAPH_COMPUTE_END);
    
    // Test operation events for specific type
    auto mul_mat_events = reader.get_op_events_for_type(26); // MUL_MAT
    assert(mul_mat_events.size() == 2); // BEGIN and END
    
    std::cout << "✓ Event filtering test passed\n";
}

// Test timing calculations
void test_timing_calculations() {
    std::cout << "\n=== Test: Timing Calculations ===\n";
    
    const char* filename = "test_timing.ggmlviz";
    create_test_trace(filename);
    
    TraceReader reader(filename);
    assert(reader.is_valid());
    
    // Test total duration
    uint64_t total_duration = reader.get_total_duration_ns();
    assert(total_duration == 100000); // 100 microseconds
    
    // Test operation timings
    auto timings = reader.get_op_timings();
    assert(timings.size() == 3); // 3 complete operations
    
    // Check sorted by duration (longest first)
    assert(timings[0].duration_ns >= timings[1].duration_ns);
    assert(timings[1].duration_ns >= timings[2].duration_ns);
    
    // Verify specific timings
    bool found_matmul = false;
    for (const auto& timing : timings) {
        if (timing.name == "matmul_AB") {
            assert(timing.duration_ns == 49000); // 50000 - 1000
            found_matmul = true;
        }
    }
    assert(found_matmul);
    
    std::cout << "✓ Timing calculations test passed\n";
}

// Test with empty trace file
void test_empty_trace() {
    std::cout << "\n=== Test: Empty Trace ===\n";
    
    const char* filename = "test_empty.ggmlviz";
    FILE* file = fopen(filename, "wb");
    assert(file != nullptr);
    
    // Write only header
    const char magic[] = "GGMLVIZ1";
    fwrite(magic, 1, 8, file);
    uint32_t version = 1;
    fwrite(&version, sizeof(version), 1, file);
    fclose(file);
    
    TraceReader reader(filename);
    assert(reader.is_valid());
    assert(reader.event_count() == 0);
    assert(reader.get_total_duration_ns() == 0);
    
    std::cout << "✓ Empty trace test passed\n";
}

// Test corrupted file handling
void test_corrupted_file() {
    std::cout << "\n=== Test: Corrupted File ===\n";
    
    const char* filename = "test_corrupted.ggmlviz";
    FILE* file = fopen(filename, "wb");
    assert(file != nullptr);
    
    // Write bad magic
    const char bad_magic[] = "BADMAGIC";
    fwrite(bad_magic, 1, 8, file);
    fclose(file);
    
    TraceReader reader(filename);
    assert(!reader.is_valid());
    
    std::cout << "✓ Corrupted file test passed\n";
}

// Performance test with large trace
void test_large_trace_performance() {
    std::cout << "\n=== Test: Large Trace Performance ===\n";
    
    const char* filename = "test_large.ggmlviz";
    FILE* file = fopen(filename, "wb");
    assert(file != nullptr);
    
    // Write header
    const char magic[] = "GGMLVIZ1";
    fwrite(magic, 1, 8, file);
    uint32_t version = 1;
    fwrite(&version, sizeof(version), 1, file);
    
    // Write many events
    const int num_events = 100000;
    uint64_t timestamp = 1000000000;
    
    auto start_write = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_events; ++i) {
        Event event = {};
        event.type = (i % 2 == 0) ? EventType::OP_COMPUTE_BEGIN : EventType::OP_COMPUTE_END;
        event.timestamp_ns = timestamp + i * 1000;
        event.thread_id = i % 4;
        event.data.op.tensor_ptr = reinterpret_cast<void*>(0x1000 + (i / 2));
        event.data.op.op_type = 26; // MUL_MAT
        event.data.op.op_size = 1024;
        
        fwrite(&event.type, sizeof(event.type), 1, file);
        fwrite(&event.timestamp_ns, sizeof(event.timestamp_ns), 1, file);
        fwrite(&event.thread_id, sizeof(event.thread_id), 1, file);
        fwrite(&event.data, sizeof(event.data), 1, file);
        
        uint8_t has_label = 0;
        fwrite(&has_label, 1, 1, file);
    }
    
    fclose(file);
    
    auto end_write = std::chrono::high_resolution_clock::now();
    auto write_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_write - start_write);
    std::cout << "  Created " << num_events << " events in " << write_duration.count() << " ms\n";
    
    // Test reading performance
    auto start_read = std::chrono::high_resolution_clock::now();
    TraceReader reader(filename);
    auto end_read = std::chrono::high_resolution_clock::now();
    auto read_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_read - start_read);
    
    assert(reader.is_valid());
    assert(reader.event_count() == num_events);
    
    std::cout << "  Read " << reader.event_count() << " events in " << read_duration.count() << " ms\n";
    std::cout << "  Read rate: " << (num_events / (read_duration.count() / 1000.0)) << " events/sec\n";
    
    // Test analysis performance
    auto start_analysis = std::chrono::high_resolution_clock::now();
    auto timings = reader.get_op_timings();
    auto end_analysis = std::chrono::high_resolution_clock::now();
    auto analysis_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_analysis - start_analysis);
    
    std::cout << "  Analyzed " << timings.size() << " operations in " << analysis_duration.count() << " ms\n";
    
    std::cout << "✓ Large trace performance test passed\n";
}

// Cleanup test files
void cleanup_test_files() {
    const char* test_files[] = {
        "test_basic.ggmlviz",
        "test_filter.ggmlviz",
        "test_timing.ggmlviz",
        "test_empty.ggmlviz",
        "test_corrupted.ggmlviz",
        "test_large.ggmlviz"
    };
    
    for (const char* filename : test_files) {
        std::remove(filename);
    }
}

int main() {
    std::cout << "Running TraceReader tests...\n";
    
    try {
        test_basic_loading();
        test_invalid_file();
        test_event_filtering();
        test_timing_calculations();
        test_empty_trace();
        test_corrupted_file();
        test_large_trace_performance();
        
        std::cout << "\n✅ All tests passed!\n";
        
        // Cleanup
        cleanup_test_files();
        
        // Also test with the real trace if it exists
        if (FILE* f = fopen("test_trace.ggmlviz", "rb")) {
            fclose(f);
            std::cout << "\n=== Testing with real trace file ===\n";
            
            TraceReader reader("test_trace.ggmlviz");
            if (reader.is_valid()) {
                std::cout << "Event count: " << reader.event_count() << "\n";
                std::cout << "Total duration: " << reader.get_total_duration_ns() / 1000000 << " ms\n";
                
                auto timings = reader.get_op_timings();
                std::cout << "\nTop 5 slowest operations:\n";
                for (size_t i = 0; i < std::min(size_t(5), timings.size()); ++i) {
                    const auto& t = timings[i];
                    std::cout << "  " << t.name << ": " 
                              << t.duration_ns / 1000 << " µs\n";
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        cleanup_test_files();
        return 1;
    }
    
    return 0;
}