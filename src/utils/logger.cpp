// src/utils/logger.cpp
#include "logger.hpp"
#include "config.hpp"
#include <cstdlib>
#include <thread>
#include <cstdio>

namespace ggml_viz {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() 
    : current_level_(LogLevel::INFO)
    , timestamp_enabled_(true)
    , thread_id_enabled_(false)
    , prefix_("[GGML_VIZ]") {
    // Configure from environment variables on creation
    configure_from_env();
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_level_ = level;
}

LogLevel Logger::get_level() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_level_;
}

void Logger::set_timestamp_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    timestamp_enabled_ = enabled;
}

void Logger::set_thread_id_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    thread_id_enabled_ = enabled;
}

void Logger::set_prefix(const std::string& prefix) {
    std::lock_guard<std::mutex> lock(mutex_);
    prefix_ = prefix;
}

void Logger::configure_from_env() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check GGML_VIZ_VERBOSE for backward compatibility
    const char* verbose = std::getenv("GGML_VIZ_VERBOSE");
    if (verbose && std::string(verbose) == "1") {
        current_level_ = LogLevel::DEBUG;
    }
    
    // Check GGML_VIZ_LOG_LEVEL for specific level
    const char* log_level = std::getenv("GGML_VIZ_LOG_LEVEL");
    if (log_level) {
        std::string level_str(log_level);
        if (level_str == "DEBUG" || level_str == "debug") {
            current_level_ = LogLevel::DEBUG;
        } else if (level_str == "INFO" || level_str == "info") {
            current_level_ = LogLevel::INFO;
        } else if (level_str == "WARN" || level_str == "warn") {
            current_level_ = LogLevel::WARN;
        } else if (level_str == "ERROR" || level_str == "error") {
            current_level_ = LogLevel::ERROR_LEVEL;
        } else if (level_str == "FATAL" || level_str == "fatal") {
            current_level_ = LogLevel::FATAL;
        }
    }
    
    // Check GGML_VIZ_LOG_TIMESTAMP
    const char* timestamp = std::getenv("GGML_VIZ_LOG_TIMESTAMP");
    if (timestamp) {
        timestamp_enabled_ = (std::string(timestamp) == "1" || std::string(timestamp) == "true");
    }
    
    // Check GGML_VIZ_LOG_THREAD_ID
    const char* thread_id = std::getenv("GGML_VIZ_LOG_THREAD_ID");
    if (thread_id) {
        thread_id_enabled_ = (std::string(thread_id) == "1" || std::string(thread_id) == "true");
    }
    
    // Check GGML_VIZ_LOG_PREFIX
    const char* prefix = std::getenv("GGML_VIZ_LOG_PREFIX");
    if (prefix) {
        prefix_ = std::string(prefix);
    }
}

void Logger::configure_from_config(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Convert ConfigLogLevel to LogLevel
    switch (config.logging.level) {
        case ConfigLogLevel::DEBUG:
            current_level_ = LogLevel::DEBUG;
            break;
        case ConfigLogLevel::INFO:
            current_level_ = LogLevel::INFO;
            break;
        case ConfigLogLevel::WARN:
            current_level_ = LogLevel::WARN;
            break;
        case ConfigLogLevel::ERROR_LEVEL:
            current_level_ = LogLevel::ERROR_LEVEL;
            break;
        case ConfigLogLevel::FATAL:
            current_level_ = LogLevel::FATAL;
            break;
    }
    
    timestamp_enabled_ = config.logging.timestamp;
    thread_id_enabled_ = config.logging.thread_id;
    prefix_ = config.logging.prefix;
}

bool Logger::should_log(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(current_level_);
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR_LEVEL: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::get_thread_id() const {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!should_log(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream oss;
    
    // Add prefix
    if (!prefix_.empty()) {
        oss << prefix_ << " ";
    }
    
    // Add timestamp
    if (timestamp_enabled_) {
        oss << "[" << get_timestamp() << "] ";
    }
    
    // Add log level
    oss << "[" << level_to_string(level) << "] ";
    
    // Add thread ID
    if (thread_id_enabled_) {
        oss << "[T:" << get_thread_id() << "] ";
    }
    
    // Add message
    oss << message;
    
    // Output to appropriate stream
    if (level >= LogLevel::ERROR_LEVEL) {
        std::cerr << oss.str() << std::endl;
    } else {
        std::cout << oss.str() << std::endl;
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR_LEVEL, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

// LogStream implementation
Logger::LogStream::LogStream(Logger& logger, LogLevel level)
    : logger_(logger), level_(level), should_log_(logger.should_log(level)) {
}

Logger::LogStream::~LogStream() {
    if (should_log_) {
        logger_.log(level_, stream_.str());
    }
}

Logger::LogStream Logger::debug_stream() {
    return LogStream(*this, LogLevel::DEBUG);
}

Logger::LogStream Logger::info_stream() {
    return LogStream(*this, LogLevel::INFO);
}

Logger::LogStream Logger::warn_stream() {
    return LogStream(*this, LogLevel::WARN);
}

Logger::LogStream Logger::error_stream() {
    return LogStream(*this, LogLevel::ERROR_LEVEL);
}

Logger::LogStream Logger::fatal_stream() {
    return LogStream(*this, LogLevel::FATAL);
}

} // namespace ggml_viz