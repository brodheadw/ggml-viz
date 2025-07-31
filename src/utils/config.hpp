// src/utils/config.hpp
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <atomic>
#include <mutex>

namespace ggml_viz {

/**
 * Configuration schema version for compatibility checking
 */
enum class ConfigVersion : uint32_t {
    V1 = 1,
    CURRENT = V1
};

/**
 * Log levels supported by the system
 */
enum class ConfigLogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR_LEVEL,
    FATAL
};

/**
 * Instrumentation configuration settings
 */
struct InstrumentationConfig {
    bool enable_op_timing = true;
    bool enable_memory_tracking = false;
    bool record_tensor_names = true;
    uint64_t max_events = 1000000;
    std::vector<uint32_t> op_types_to_trace;  // Empty = trace all
    std::vector<std::string> backends_to_trace = {"cpu", "metal", "cuda"};
};

/**
 * Output configuration settings
 */
struct OutputConfig {
    std::string filename = "ggml_trace.ggmlviz";
    bool write_to_file = true;
    uint32_t flush_interval = 4096;  // Events between flushes
};

/**
 * Logging configuration settings
 */
struct LoggingConfig {
    ConfigLogLevel level = ConfigLogLevel::INFO;
    bool timestamp = true;
    bool thread_id = false;
    std::string prefix = "[GGML_VIZ]";
};

/**
 * UI configuration settings
 */
struct UIConfig {
    bool live_mode = false;
    uint32_t poll_interval_ms = 100;
    uint32_t max_live_events = 50000;
};

/**
 * Complete configuration structure
 */
struct Config {
    ConfigVersion schema_version = ConfigVersion::CURRENT;
    InstrumentationConfig instrumentation;
    OutputConfig output;
    LoggingConfig logging;
    UIConfig ui;
    
    // Validation
    bool is_valid() const;
    std::string validation_error() const;
    
    // Serialization
    std::string to_json() const;
    static Config from_json(const std::string& json_str);
    static Config from_file(const std::string& filepath);
    
    // Merging (for precedence handling)
    void merge_from(const Config& other);
    
    // Environment variable fallbacks
    void apply_env_overrides();
    
    // Default configuration
    static Config default_config();
};

/**
 * Thread-safe configuration singleton
 */
class ConfigManager {
public:
    static ConfigManager& instance();
    
    // Load configuration with precedence: CLI > file > env > defaults
    void load_with_precedence(
        const std::string& cli_config_path = "",
        const std::string& env_config_path = "",
        const std::string& default_config_path = "ggml-viz.json"
    );
    
    // Access current configuration (lock-free for hot path)
    std::shared_ptr<const Config> get() const noexcept;
    
    // Check if configuration has been loaded
    bool is_loaded() const;
    
    // Dump current config as JSON (for --dump-config)
    std::string dump_json() const;
    
    // Reset to unloaded state (for testing)
    void reset();

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    mutable std::mutex mutex_;  // Only used for loading/reset operations
    mutable std::shared_ptr<const Config> config_ptr_;
    std::atomic<bool> loaded_{false};
};

} // namespace ggml_viz