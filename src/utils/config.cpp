// src/utils/config.cpp
#include "config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <mutex>
#include <cstdlib>
#include <iostream>

using json = nlohmann::json;

namespace ggml_viz {

// Helper function to convert ConfigLogLevel to/from string
static std::string log_level_to_string(ConfigLogLevel level) {
    switch (level) {
        case ConfigLogLevel::DEBUG: return "DEBUG";
        case ConfigLogLevel::INFO: return "INFO";
        case ConfigLogLevel::WARN: return "WARN";
        case ConfigLogLevel::ERROR_LEVEL: return "ERROR";
        case ConfigLogLevel::FATAL: return "FATAL";
        default: return "INFO";
    }
}

static ConfigLogLevel string_to_log_level(const std::string& str) {
    if (str == "DEBUG") return ConfigLogLevel::DEBUG;
    if (str == "INFO") return ConfigLogLevel::INFO;
    if (str == "WARN") return ConfigLogLevel::WARN;
    if (str == "ERROR") return ConfigLogLevel::ERROR_LEVEL;
    if (str == "FATAL") return ConfigLogLevel::FATAL;
    return ConfigLogLevel::INFO; // Default fallback
}

// Config validation
bool Config::is_valid() const {
    return validation_error().empty();
}

std::string Config::validation_error() const {
    // Schema version check
    if (schema_version != ConfigVersion::V1) {
        return "Unsupported schema version: " + std::to_string(static_cast<uint32_t>(schema_version));
    }
    
    // Instrumentation validation
    if (instrumentation.max_events == 0) {
        return "instrumentation.max_events must be greater than 0";
    }
    
    if (instrumentation.max_events > 100000000) { // 100M limit
        return "instrumentation.max_events exceeds maximum (100M)";
    }
    
    // Output validation
    if (output.filename.empty()) {
        return "output.filename cannot be empty";
    }
    
    if (output.flush_interval == 0) {
        return "output.flush_interval must be greater than 0";
    }
    
    // UI validation
    if (ui.poll_interval_ms == 0) {
        return "ui.poll_interval_ms must be greater than 0";
    }
    
    if (ui.max_live_events == 0) {
        return "ui.max_live_events must be greater than 0";
    }
    
    return ""; // Valid
}

// JSON serialization
std::string Config::to_json() const {
    json j;
    
    j["schema_version"] = static_cast<uint32_t>(schema_version);
    
    // Instrumentation
    j["instrumentation"]["enable_op_timing"] = instrumentation.enable_op_timing;
    j["instrumentation"]["enable_memory_tracking"] = instrumentation.enable_memory_tracking;
    j["instrumentation"]["record_tensor_names"] = instrumentation.record_tensor_names;
    j["instrumentation"]["max_events"] = instrumentation.max_events;
    j["instrumentation"]["op_types_to_trace"] = instrumentation.op_types_to_trace;
    j["instrumentation"]["backends_to_trace"] = instrumentation.backends_to_trace;
    
    // Output
    j["output"]["filename"] = output.filename;
    j["output"]["write_to_file"] = output.write_to_file;
    j["output"]["flush_interval"] = output.flush_interval;
    
    // Logging
    j["logging"]["level"] = log_level_to_string(logging.level);
    j["logging"]["timestamp"] = logging.timestamp;
    j["logging"]["thread_id"] = logging.thread_id;
    j["logging"]["prefix"] = logging.prefix;
    
    // UI
    j["ui"]["live_mode"] = ui.live_mode;
    j["ui"]["poll_interval_ms"] = ui.poll_interval_ms;
    j["ui"]["max_live_events"] = ui.max_live_events;
    
    return j.dump(2); // Pretty print with 2-space indent
}

// JSON deserialization
Config Config::from_json(const std::string& json_str) {
    Config config = default_config();
    
    try {
        json j = json::parse(json_str);
        
        // Schema version (required)
        if (j.contains("schema_version")) {
            uint32_t version = j["schema_version"];
            if (version != static_cast<uint32_t>(ConfigVersion::V1)) {
                throw std::runtime_error("Unsupported schema_version: " + std::to_string(version));
            }
            config.schema_version = static_cast<ConfigVersion>(version);
        }
        
        // Instrumentation (optional fields)
        if (j.contains("instrumentation")) {
            auto& inst = j["instrumentation"];
            if (inst.contains("enable_op_timing")) {
                config.instrumentation.enable_op_timing = inst["enable_op_timing"];
            }
            if (inst.contains("enable_memory_tracking")) {
                config.instrumentation.enable_memory_tracking = inst["enable_memory_tracking"];
            }
            if (inst.contains("record_tensor_names")) {
                config.instrumentation.record_tensor_names = inst["record_tensor_names"];
            }
            if (inst.contains("max_events")) {
                config.instrumentation.max_events = inst["max_events"];
            }
            if (inst.contains("op_types_to_trace")) {
                config.instrumentation.op_types_to_trace = inst["op_types_to_trace"].get<std::vector<uint32_t>>();
            }
            if (inst.contains("backends_to_trace")) {
                config.instrumentation.backends_to_trace = inst["backends_to_trace"].get<std::vector<std::string>>();
            }
        }
        
        // Output
        if (j.contains("output")) {
            auto& out = j["output"];
            if (out.contains("filename")) {
                config.output.filename = out["filename"];
            }
            if (out.contains("write_to_file")) {
                config.output.write_to_file = out["write_to_file"];
            }
            if (out.contains("flush_interval")) {
                config.output.flush_interval = out["flush_interval"];
            }
        }
        
        // Logging  
        if (j.contains("logging")) {
            auto& log = j["logging"];
            if (log.contains("level")) {
                std::string level_str = log["level"];
                config.logging.level = string_to_log_level(level_str);
            }
            if (log.contains("timestamp")) {
                config.logging.timestamp = log["timestamp"];
            }
            if (log.contains("thread_id")) {
                config.logging.thread_id = log["thread_id"];
            }
            if (log.contains("prefix")) {
                config.logging.prefix = log["prefix"];
            }
        }
        
        // UI
        if (j.contains("ui")) {
            auto& ui_cfg = j["ui"];
            if (ui_cfg.contains("live_mode")) {
                config.ui.live_mode = ui_cfg["live_mode"];
            }
            if (ui_cfg.contains("poll_interval_ms")) {
                config.ui.poll_interval_ms = ui_cfg["poll_interval_ms"];
            }
            if (ui_cfg.contains("max_live_events")) {
                config.ui.max_live_events = ui_cfg["max_live_events"];
            }
        }
        
        // Warn about unknown keys
        std::vector<std::string> known_keys = {
            "schema_version", "instrumentation", "output", "logging", "ui"
        };
        
        for (auto& [key, value] : j.items()) {
            if (std::find(known_keys.begin(), known_keys.end(), key) == known_keys.end()) {
                std::cerr << "[GGML_VIZ] Warning: Unknown config key '" << key 
                         << "' - will be ignored\n";
            }
        }
        
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }
    
    // Validate the final config
    if (!config.is_valid()) {
        throw std::runtime_error("Invalid configuration: " + config.validation_error());
    }
    
    return config;
}

Config Config::from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + filepath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_json(buffer.str());
}

// Merging for precedence handling
void Config::merge_from(const Config& other) {
    // Only merge if other config has different values from defaults
    Config defaults = default_config();
    
    // Instrumentation
    if (other.instrumentation.enable_op_timing != defaults.instrumentation.enable_op_timing) {
        instrumentation.enable_op_timing = other.instrumentation.enable_op_timing;
    }
    if (other.instrumentation.enable_memory_tracking != defaults.instrumentation.enable_memory_tracking) {
        instrumentation.enable_memory_tracking = other.instrumentation.enable_memory_tracking;
    }
    if (other.instrumentation.record_tensor_names != defaults.instrumentation.record_tensor_names) {
        instrumentation.record_tensor_names = other.instrumentation.record_tensor_names;
    }
    if (other.instrumentation.max_events != defaults.instrumentation.max_events) {
        instrumentation.max_events = other.instrumentation.max_events;
    }
    if (!other.instrumentation.op_types_to_trace.empty()) {
        instrumentation.op_types_to_trace = other.instrumentation.op_types_to_trace;
    }
    if (other.instrumentation.backends_to_trace != defaults.instrumentation.backends_to_trace) {
        instrumentation.backends_to_trace = other.instrumentation.backends_to_trace;
    }
    
    // Output
    if (other.output.filename != defaults.output.filename) {
        output.filename = other.output.filename;
    }
    if (other.output.write_to_file != defaults.output.write_to_file) {
        output.write_to_file = other.output.write_to_file;
    }
    if (other.output.flush_interval != defaults.output.flush_interval) {
        output.flush_interval = other.output.flush_interval;
    }
    
    // Logging
    if (other.logging.level != defaults.logging.level) {
        logging.level = other.logging.level;
    }
    if (other.logging.timestamp != defaults.logging.timestamp) {
        logging.timestamp = other.logging.timestamp;
    }
    if (other.logging.thread_id != defaults.logging.thread_id) {
        logging.thread_id = other.logging.thread_id;
    }
    if (other.logging.prefix != defaults.logging.prefix) {
        logging.prefix = other.logging.prefix;
    }
    
    // UI
    if (other.ui.live_mode != defaults.ui.live_mode) {
        ui.live_mode = other.ui.live_mode;
    }
    if (other.ui.poll_interval_ms != defaults.ui.poll_interval_ms) {
        ui.poll_interval_ms = other.ui.poll_interval_ms;
    }
    if (other.ui.max_live_events != defaults.ui.max_live_events) {
        ui.max_live_events = other.ui.max_live_events;
    }
}

// Environment variable overrides
void Config::apply_env_overrides() {
    const char* env_output = std::getenv("GGML_VIZ_OUTPUT");
    if (env_output) {
        output.filename = env_output;
        output.write_to_file = true;
    }
    
    const char* env_max_events = std::getenv("GGML_VIZ_MAX_EVENTS");
    if (env_max_events) {
        try {
            instrumentation.max_events = std::stoull(env_max_events);
        } catch (...) {
            std::cerr << "[GGML_VIZ] Warning: Invalid GGML_VIZ_MAX_EVENTS value, using default\n";
        }
    }
    
    const char* env_verbose = std::getenv("GGML_VIZ_VERBOSE");
    if (env_verbose && (std::string(env_verbose) == "1" || std::string(env_verbose) == "true")) {
        logging.level = ConfigLogLevel::DEBUG;
    }
    
    const char* env_disable = std::getenv("GGML_VIZ_DISABLE");
    if (env_disable && (std::string(env_disable) == "1" || std::string(env_disable) == "true")) {
        instrumentation.enable_op_timing = false;
        instrumentation.enable_memory_tracking = false;
        output.write_to_file = false;
    }
}

// Default configuration
Config Config::default_config() {
    Config config;
    // All defaults are set in struct definitions
    return config;
}

// ConfigManager implementation
ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::load_with_precedence(
    const std::string& cli_config_path,
    const std::string& env_config_path,
    const std::string& default_config_path
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Start with defaults
    config_ = Config::default_config();
    
    // Apply in precedence order: defaults < file < env < CLI
    
    // 1. Try default config file
    if (!default_config_path.empty()) {
        try {
            Config file_config = Config::from_file(default_config_path);
            config_.merge_from(file_config);
            std::cout << "[GGML_VIZ] Loaded config from: " << default_config_path << "\n";
        } catch (const std::exception& e) {
            // Default config file is optional
        }
    }
    
    // 2. Try environment-specified config file
    if (!env_config_path.empty()) {
        try {
            Config env_file_config = Config::from_file(env_config_path);
            config_.merge_from(env_file_config);
            std::cout << "[GGML_VIZ] Loaded config from env: " << env_config_path << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[GGML_VIZ] Warning: Could not load env config " 
                     << env_config_path << ": " << e.what() << "\n";
        }
    }
    
    // 3. Apply environment variable overrides
    config_.apply_env_overrides();
    
    // 4. Try CLI-specified config file (highest precedence)
    if (!cli_config_path.empty()) {
        try {
            Config cli_config = Config::from_file(cli_config_path);
            config_.merge_from(cli_config);
            std::cout << "[GGML_VIZ] Loaded config from CLI: " << cli_config_path << "\n";
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to load CLI config " + cli_config_path + ": " + e.what());
        }
    }
    
    // Final validation
    if (!config_.is_valid()) {
        throw std::runtime_error("Final configuration is invalid: " + config_.validation_error());
    }
    
    loaded_ = true;
    
    // Log resolved settings at INFO level
    std::cout << "[GGML_VIZ] Configuration loaded successfully:\n";
    std::cout << "[GGML_VIZ]   Output file: " << config_.output.filename << "\n";
    std::cout << "[GGML_VIZ]   Max events: " << config_.instrumentation.max_events << "\n";
    std::cout << "[GGML_VIZ]   Log level: " << log_level_to_string(config_.logging.level) << "\n";
    std::cout << "[GGML_VIZ]   Op timing: " << (config_.instrumentation.enable_op_timing ? "enabled" : "disabled") << "\n";
}

const Config& ConfigManager::get() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!loaded_) {
        throw std::runtime_error("Configuration not loaded - call load_with_precedence() first");
    }
    return config_;
}

bool ConfigManager::is_loaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return loaded_;
}

std::string ConfigManager::dump_json() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!loaded_) {
        return Config::default_config().to_json();
    }
    return config_.to_json();
}

void ConfigManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = Config::default_config();
    loaded_ = false;
}

} // namespace ggml_viz