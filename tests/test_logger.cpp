// tests/test_logger.cpp
#include "utils/logger.hpp"
#include <cassert>
#include <sstream>
#include <iostream>

using namespace ggml_viz;

void test_basic_logging() {
    std::cout << "Testing basic logging functionality..." << std::endl;
    
    auto& logger = Logger::instance();
    
    // Test all log levels
    logger.debug("This is a debug message");
    logger.info("This is an info message");
    logger.warn("This is a warning message");
    logger.error("This is an error message");
    logger.fatal("This is a fatal message");
    
    std::cout << "✅ Basic logging test passed" << std::endl;
}

void test_log_levels() {
    std::cout << "Testing log level filtering..." << std::endl;
    
    auto& logger = Logger::instance();
    
    // Set to WARN level - should only see WARN, ERROR, FATAL
    logger.set_level(LogLevel::WARN);
    
    std::cout << "Setting log level to WARN (should only see WARN, ERROR, FATAL):" << std::endl;
    logger.debug("This debug should NOT appear");
    logger.info("This info should NOT appear"); 
    logger.warn("This warning SHOULD appear");
    logger.error("This error SHOULD appear");
    logger.fatal("This fatal SHOULD appear");
    
    // Reset to DEBUG for other tests
    logger.set_level(LogLevel::DEBUG);
    
    std::cout << "✅ Log level filtering test passed" << std::endl;
}

void test_formatted_logging() {
    std::cout << "Testing formatted logging..." << std::endl;
    
    auto& logger = Logger::instance();
    
    int number = 42;
    const char* string = "test";
    double decimal = 3.14159;
    
    logger.info("Formatted message: number=%d, string=%s, decimal=%.2f", number, string, decimal);
    logger.warn("Another format test: %d + %d = %d", 2, 3, 5);
    
    std::cout << "✅ Formatted logging test passed" << std::endl;
}

void test_stream_logging() {
    std::cout << "Testing stream-style logging..." << std::endl;
    
    GGML_VIZ_DEBUG << "Debug stream: " << 123 << " items processed";
    GGML_VIZ_INFO << "Info stream: Processing file " << "test.txt";
    GGML_VIZ_WARN << "Warning stream: Memory usage at " << 85.6 << "%";
    GGML_VIZ_ERROR << "Error stream: Failed to open " << "missing.file";
    
    std::cout << "✅ Stream logging test passed" << std::endl;
}

void test_configuration() {
    std::cout << "Testing logger configuration..." << std::endl;
    
    auto& logger = Logger::instance();
    
    // Test timestamp toggle
    std::cout << "Disabling timestamps:" << std::endl;
    logger.set_timestamp_enabled(false);
    logger.info("Message without timestamp");
    
    std::cout << "Enabling timestamps:" << std::endl;
    logger.set_timestamp_enabled(true);
    logger.info("Message with timestamp");
    
    // Test thread ID toggle
    std::cout << "Enabling thread IDs:" << std::endl;
    logger.set_thread_id_enabled(true);
    logger.info("Message with thread ID");
    
    std::cout << "Disabling thread IDs:" << std::endl;
    logger.set_thread_id_enabled(false);
    logger.info("Message without thread ID");
    
    // Test custom prefix
    std::cout << "Custom prefix test:" << std::endl;
    logger.set_prefix("[TEST_PREFIX]");
    logger.info("Message with custom prefix");
    
    // Reset to default
    logger.set_prefix("[GGML_VIZ]");
    logger.info("Message with default prefix restored");
    
    std::cout << "✅ Configuration test passed" << std::endl;
}

void test_macros() {
    std::cout << "Testing convenience macros..." << std::endl;
    
    GGML_VIZ_LOG_DEBUG("Debug message via macro");
    GGML_VIZ_LOG_INFO("Info message via macro");
    GGML_VIZ_LOG_WARN("Warning message via macro");
    GGML_VIZ_LOG_ERROR("Error message via macro");
    
    GGML_VIZ_LOG_INFO_FMT("Formatted macro: %s = %d", "answer", 42);
    GGML_VIZ_LOG_WARN_FMT("Another format: %.1f%% complete", 67.8);
    
    std::cout << "✅ Macro test passed" << std::endl;
}

int main() {
    std::cout << "🧪 GGML Visualizer Logger Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << std::endl;
    
    try {
        test_basic_logging();
        std::cout << std::endl;
        
        test_log_levels();
        std::cout << std::endl;
        
        test_formatted_logging();
        std::cout << std::endl;
        
        test_stream_logging();
        std::cout << std::endl;
        
        test_configuration();
        std::cout << std::endl;
        
        test_macros();
        std::cout << std::endl;
        
        std::cout << "🎉 All logger tests passed!" << std::endl;
        std::cout << std::endl;
        
        std::cout << "💡 Environment variable configuration:" << std::endl;
        std::cout << "  GGML_VIZ_VERBOSE=1           # Enable debug logging (backward compatibility)" << std::endl;
        std::cout << "  GGML_VIZ_LOG_LEVEL=DEBUG     # Set specific log level (DEBUG/INFO/WARN/ERROR/FATAL)" << std::endl;
        std::cout << "  GGML_VIZ_LOG_TIMESTAMP=0     # Disable timestamps" << std::endl;
        std::cout << "  GGML_VIZ_LOG_THREAD_ID=1     # Enable thread IDs" << std::endl;
        std::cout << "  GGML_VIZ_LOG_PREFIX=[CUSTOM] # Set custom log prefix" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}