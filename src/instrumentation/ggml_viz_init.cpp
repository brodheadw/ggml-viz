// src/instrumentation/ggml_viz_init.cpp
#include "ggml_hook.hpp"
#include "../utils/logger.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

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
            
            // Settings come from ConfigManager (env vars and config files), loaded by start()
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
                std::cerr << "[GGML_VIZ] Status: Active (events recorded: " 
                         << GGMLHook::instance().event_count() << ")" << std::endl;
            } else {
                std::cerr << "[GGML_VIZ] Status: Initialized but not active" << std::endl;
            }
        } else {
            std::cerr << "[GGML_VIZ] Status: Not initialized" << std::endl;
        }
    }
}

} // namespace ggml_viz