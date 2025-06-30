// test_injection.cpp - Simple program to test GGML Viz auto-injection
#include <iostream>
#include <chrono>
#include <thread>

// Declare the status function from our shared library
extern "C" {
    bool ggml_viz_is_initialized();
    void ggml_viz_print_status();
}

int main() {
    std::cout << "=== GGML Visualizer Injection Test ===" << std::endl;
    
    // Give some time for auto-initialization
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check if the visualizer was initialized
    std::cout << "Checking initialization status..." << std::endl;
    
    try {
        if (ggml_viz_is_initialized()) {
            std::cout << "✅ GGML Visualizer is initialized!" << std::endl;
        } else {
            std::cout << "❌ GGML Visualizer is NOT initialized" << std::endl;
        }
        
        // Print detailed status
        std::cout << "\nDetailed status:" << std::endl;
        ggml_viz_print_status();
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error calling status functions: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "❌ Unknown error calling status functions" << std::endl;
    }
    
    std::cout << "\nTest completed. Exiting..." << std::endl;
    return 0;
}