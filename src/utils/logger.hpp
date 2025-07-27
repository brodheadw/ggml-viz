// src/utils/logger.hpp
#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef ERROR  // Windows defines ERROR as a macro, which conflicts with our enum
#endif

#include <string>
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>

namespace ggml_viz {

/**
 * Log levels in order of severity
 */
enum class LogLevel : int {
    DEBUG = 0,    // Detailed debug information
    INFO = 1,     // General information  
    WARN = 2,     // Warning messages
    ERROR_LEVEL = 3,    // Error messages (renamed to avoid Windows ERROR macro conflict)
    FATAL = 4     // Fatal errors
};

/**
 * Thread-safe logger with configurable output and formatting
 */
class Logger {
public:
    /**
     * Get the global logger instance
     */
    static Logger& instance();

    /**
     * Set the minimum log level (logs below this level are ignored)
     */
    void set_level(LogLevel level);

    /**
     * Get the current log level
     */
    LogLevel get_level() const;

    /**
     * Enable/disable timestamp in log output
     */
    void set_timestamp_enabled(bool enabled);

    /**
     * Enable/disable thread ID in log output
     */
    void set_thread_id_enabled(bool enabled);

    /**
     * Set custom log prefix (default: "[GGML_VIZ]")
     */
    void set_prefix(const std::string& prefix);

    /**
     * Configure logger from environment variables
     */
    void configure_from_env();

    /**
     * Log a message at the specified level
     */
    void log(LogLevel level, const std::string& message);

    /**
     * Convenience methods for different log levels
     */
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

    /**
     * Formatted logging with printf-style arguments
     */
    template<typename... Args>
    void debug(const std::string& format, Args... args) {
        if (should_log(LogLevel::DEBUG)) {
            log(LogLevel::DEBUG, format_string(format, args...));
        }
    }

    template<typename... Args>
    void info(const std::string& format, Args... args) {
        if (should_log(LogLevel::INFO)) {
            log(LogLevel::INFO, format_string(format, args...));
        }
    }

    template<typename... Args>
    void warn(const std::string& format, Args... args) {
        if (should_log(LogLevel::WARN)) {
            log(LogLevel::WARN, format_string(format, args...));
        }
    }

    template<typename... Args>
    void error(const std::string& format, Args... args) {
        if (should_log(LogLevel::ERROR_LEVEL)) {
            log(LogLevel::ERROR_LEVEL, format_string(format, args...));
        }
    }

    template<typename... Args>
    void fatal(const std::string& format, Args... args) {
        if (should_log(LogLevel::FATAL)) {
            log(LogLevel::FATAL, format_string(format, args...));
        }
    }

    /**
     * Stream-style logging interface
     */
    class LogStream {
    public:
        LogStream(Logger& logger, LogLevel level);
        ~LogStream();

        template<typename T>
        LogStream& operator<<(const T& value) {
            if (should_log_) {
                stream_ << value;
            }
            return *this;
        }

    private:
        Logger& logger_;
        LogLevel level_;
        bool should_log_;
        std::ostringstream stream_;
    };

    /**
     * Create log streams for different levels
     */
    LogStream debug_stream();
    LogStream info_stream();
    LogStream warn_stream();
    LogStream error_stream();
    LogStream fatal_stream();

private:
    Logger();
    ~Logger() = default;

    // Non-copyable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool should_log(LogLevel level) const;
    std::string level_to_string(LogLevel level) const;
    std::string get_timestamp() const;
    std::string get_thread_id() const;

    template<typename... Args>
    std::string format_string(const std::string& format, Args... args) {
        size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args...);
        return std::string(buf.get(), size - 1);
    }

    mutable std::mutex mutex_;
    LogLevel current_level_;
    bool timestamp_enabled_;
    bool thread_id_enabled_;
    std::string prefix_;
};

} // namespace ggml_viz

// Convenience macros for easy logging
#define GGML_VIZ_LOG_DEBUG(msg) ggml_viz::Logger::instance().debug(msg)
#define GGML_VIZ_LOG_INFO(msg) ggml_viz::Logger::instance().info(msg)
#define GGML_VIZ_LOG_WARN(msg) ggml_viz::Logger::instance().warn(msg)
#define GGML_VIZ_LOG_ERROR(msg) ggml_viz::Logger::instance().error(msg)
#define GGML_VIZ_LOG_FATAL(msg) ggml_viz::Logger::instance().fatal(msg)

// Stream-style macros
#define GGML_VIZ_DEBUG ggml_viz::Logger::instance().debug_stream()
#define GGML_VIZ_INFO ggml_viz::Logger::instance().info_stream()
#define GGML_VIZ_WARN ggml_viz::Logger::instance().warn_stream()
#define GGML_VIZ_ERROR ggml_viz::Logger::instance().error_stream()
#define GGML_VIZ_FATAL ggml_viz::Logger::instance().fatal_stream()

// Formatted logging macros
#define GGML_VIZ_LOG_DEBUG_FMT(...) ggml_viz::Logger::instance().debug(__VA_ARGS__)
#define GGML_VIZ_LOG_INFO_FMT(...) ggml_viz::Logger::instance().info(__VA_ARGS__)
#define GGML_VIZ_LOG_WARN_FMT(...) ggml_viz::Logger::instance().warn(__VA_ARGS__)
#define GGML_VIZ_LOG_ERROR_FMT(...) ggml_viz::Logger::instance().error(__VA_ARGS__)
#define GGML_VIZ_LOG_FATAL_FMT(...) ggml_viz::Logger::instance().fatal(__VA_ARGS__)