// src/instrumentation/ggml_viz_init.cpp
#include "ggml_hook.hpp"
#include "../utils/logger.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#ifndef _WIN32
#include <signal.h>
#endif

namespace ggml_viz {

static bool g_auto_initialized = false;
static bool g_verbose_logging = false;

// Helper function to parse environment variable as boolean
static bool parse_bool_env(const char* var_name, bool default_value = false) {
    const char* value = getenv(var_name);
    if (!value) return default_value;
    
    std::string str_value = value;
    return (str_value == "1" || str_value == "true" || str_value == "TRUE" || str_value == "on" || str_value == "ON");
}

// Helper function to parse environment variable as integer
static int parse_int_env(const char* var_name, int default_value = 0) {
    const char* value = getenv(var_name);
    if (!value) return default_value;
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        if (g_verbose_logging) {
            std::cerr << "[GGML_VIZ] Warning: Invalid integer value for " << var_name << ": " << value << std::endl;
        }
        return default_value;
    }
}

// Helper function to log messages (deprecated - use Logger directly)
static void log_message(const std::string& message, bool is_error = false) {
    auto& logger = Logger::instance();
    if (is_error) {
        logger.error(message);
    } else {
        logger.info(message);
    }
}

// This constructor runs automatically when the library is loaded
struct AutoInitializer {
    AutoInitializer() {
        // Check if auto-init is disabled
        if (parse_bool_env("GGML_VIZ_DISABLE")) {
            return;
        }
        
        // Set up verbose logging early
        g_verbose_logging = parse_bool_env("GGML_VIZ_VERBOSE");
        
        const char* output_file = getenv("GGML_VIZ_OUTPUT");
        if (!output_file) {
            log_message("GGML_VIZ_OUTPUT not set - instrumentation disabled");
            return;
        }
        
        if (g_auto_initialized) {
            log_message("Already initialized - skipping duplicate initialization");
            return;
        }
        
        try {
            g_auto_initialized = true;
            
            // Parse configuration from environment variables
            HookConfig config;
            config.enable_op_timing = parse_bool_env("GGML_VIZ_OP_TIMING", true);
            config.enable_memory_tracking = parse_bool_env("GGML_VIZ_MEMORY_TRACKING", false);
            config.enable_thread_tracking = parse_bool_env("GGML_VIZ_THREAD_TRACKING", false);
            config.enable_tensor_names = parse_bool_env("GGML_VIZ_TENSOR_NAMES", true);
            config.write_to_file = true;
            config.output_filename = output_file;
            config.max_events = parse_int_env("GGML_VIZ_MAX_EVENTS", 10000000); // 10M events default
            
            log_message("Initializing GGML visualization with configuration:");
            log_message("  Output file: " + std::string(output_file));
            log_message("  Op timing: " + std::string(config.enable_op_timing ? "enabled" : "disabled"));
            log_message("  Memory tracking: " + std::string(config.enable_memory_tracking ? "enabled" : "disabled"));
            log_message("  Thread tracking: " + std::string(config.enable_thread_tracking ? "enabled" : "disabled"));
            log_message("  Tensor names: " + std::string(config.enable_tensor_names ? "enabled" : "disabled"));
            log_message("  Max events: " + std::to_string(config.max_events));
            
            // Initialize the hook
            GGMLHook::instance().configure(config);
            GGMLHook::instance().start();
            
            // Install the GGML hooks for function interception
            if (!install_ggml_hooks()) {
                log_message("Warning: Failed to install GGML hooks - some functionality may be limited", true);
            }
            
            log_message("Auto-initialized successfully - tracing active");
            
            // Register cleanup handlers
            std::atexit([]() {
                if (GGMLHook::instance().is_active()) {
                    log_message("Application exiting - finalizing trace...");
                    
                    // Give a moment for any final operations
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                    GGMLHook::instance().stop();
                    log_message("Trace finalized and saved");
                }
            });
            
            // Additional cleanup for abnormal termination (Unix signals)
            #ifndef _WIN32
            signal(SIGINT, [](int) {
                log_message("Received SIGINT - saving trace before exit");
                if (GGMLHook::instance().is_active()) {
                    GGMLHook::instance().stop();
                }
                std::exit(0);
            });
            
            signal(SIGTERM, [](int) {
                log_message("Received SIGTERM - saving trace before exit");
                if (GGMLHook::instance().is_active()) {
                    GGMLHook::instance().stop();
                }
                std::exit(0);
            });
            #endif
            
        } catch (const std::exception& e) {
            log_message("Exception during initialization: " + std::string(e.what()), true);
            g_auto_initialized = false;
        } catch (...) {
            log_message("Unknown exception during initialization", true);
            g_auto_initialized = false;
        }
    }
};

// This will run automatically when the library loads
static AutoInitializer auto_init;

// Optional: Export a function to check if auto-init was successful
extern "C" {
    bool ggml_viz_is_initialized() {
        return g_auto_initialized && GGMLHook::instance().is_active();
    }
    
    void ggml_viz_print_status() {
        if (g_auto_initialized) {
            if (GGMLHook::instance().is_active()) {
                std::cout << "[GGML_VIZ] Status: Active (events recorded: " 
                         << GGMLHook::instance().event_count() << ")" << std::endl;
            } else {
                std::cout << "[GGML_VIZ] Status: Initialized but not active" << std::endl;
            }
        } else {
            std::cout << "[GGML_VIZ] Status: Not initialized" << std::endl;
        }
    }
}

} // namespace ggml_viz