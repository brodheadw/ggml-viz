# GGML Visualizer Logging System Usage

The GGML Visualizer now includes a comprehensive logging system for debugging and monitoring. This document shows how to use the logging features.

## Quick Start

```cpp
#include "utils/logger.hpp"

// Basic logging
GGML_VIZ_LOG_INFO("Application started");
GGML_VIZ_LOG_ERROR("Failed to open file: config.json");

// Formatted logging
GGML_VIZ_LOG_INFO_FMT("Processing %d events in %.2f seconds", 1000, 2.5);

// Stream-style logging
GGML_VIZ_INFO << "Processing file: " << filename << " (size: " << filesize << " bytes)";
```

## Log Levels

The logger supports 5 log levels in order of severity:
- **DEBUG**: Detailed debug information
- **INFO**: General information  
- **WARN**: Warning messages
- **ERROR**: Error messages
- **FATAL**: Fatal errors

## Configuration Methods

### JSON Configuration Files (Recommended)

Use JSON configuration files for structured, repeatable logging configuration:

```json
{
  "logging": {
    "level": "DEBUG",
    "enable_timestamps": true,
    "enable_thread_id": true,
    "prefix": "[GGML_VIZ]"
  }
}
```

Load configuration with precedence handling:
```bash
# CLI flag takes highest precedence
./bin/ggml-viz --config myconfig.json --verbose

# ConfigManager handles precedence automatically:
# 1. CLI flags (--verbose, --config)
# 2. JSON configuration file
# 3. Environment variables
# 4. Built-in defaults
```

### Environment Variable Configuration

Control logging behavior via environment variables (legacy approach):

```bash
# Enable debug logging (backward compatibility)
export GGML_VIZ_VERBOSE=1

# Set specific log level
export GGML_VIZ_LOG_LEVEL=DEBUG    # DEBUG/INFO/WARN/ERROR/FATAL

# Disable timestamps
export GGML_VIZ_LOG_TIMESTAMP=0

# Enable thread IDs  
export GGML_VIZ_LOG_THREAD_ID=1

# Set custom log prefix
export GGML_VIZ_LOG_PREFIX="[MY_APP]"
```

## Usage Examples

### Basic Logging
```cpp
auto& logger = ggml_viz::Logger::instance();
logger.info("Application initialized");
logger.warn("Memory usage high: 85%");
logger.error("Failed to connect to server");
```

### Convenience Macros
```cpp
GGML_VIZ_LOG_DEBUG("Entering function foo()");
GGML_VIZ_LOG_INFO("Configuration loaded successfully");
GGML_VIZ_LOG_WARN("Deprecated API call detected");
GGML_VIZ_LOG_ERROR("Invalid input parameter");
GGML_VIZ_LOG_FATAL("Critical system failure");
```

### Formatted Logging
```cpp
GGML_VIZ_LOG_INFO_FMT("Loaded %d models in %.2f ms", model_count, load_time);
GGML_VIZ_LOG_ERROR_FMT("Port %d already in use", port_number);
```

### Stream-Style Logging
```cpp
GGML_VIZ_DEBUG << "Processing tensor: " << tensor_name 
               << " (shape: " << shape[0] << "x" << shape[1] << ")";

GGML_VIZ_WARN << "Memory usage: " << memory_used << "/" << memory_total 
              << " (" << (memory_used * 100.0 / memory_total) << "%)";
```

### Programmatic Configuration

```cpp
#include "utils/config.hpp"
#include "utils/logger.hpp"

// ConfigManager integration (recommended)
auto& config_mgr = ggml_viz::ConfigManager::instance();
config_mgr.load_with_precedence("config.json", "", "");
auto config = config_mgr.get();

// Configure logger from config
auto& logger = ggml_viz::Logger::instance();
logger.configure_from_config(*config);

// Direct configuration (legacy approach)
logger.set_level(ggml_viz::LogLevel::DEBUG);
logger.set_timestamp_enabled(true);
logger.set_thread_id_enabled(false);
logger.set_prefix("[GGML_VIZ]");

// Reconfigure from environment
logger.configure_from_env();
```

## Integration Examples

### Error Handling
```cpp
try {
    load_config_file(filename);
    GGML_VIZ_LOG_INFO_FMT("Configuration loaded from: %s", filename.c_str());
} catch (const std::exception& e) {
    GGML_VIZ_LOG_ERROR_FMT("Failed to load config: %s", e.what());
    return false;
}
```

### Performance Monitoring
```cpp
auto start = std::chrono::high_resolution_clock::now();
process_data();
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

GGML_VIZ_LOG_INFO_FMT("Data processing completed in %d ms", duration.count());
```

### Conditional Logging
```cpp
// Only log debug info if debug level is enabled
if (logger.get_level() <= ggml_viz::LogLevel::DEBUG) {
    GGML_VIZ_LOG_DEBUG_FMT("Detailed state: %s", get_detailed_state().c_str());
}
```

## Output Format

The default log format is:
```
[PREFIX] [TIMESTAMP] [LEVEL] [THREAD_ID] MESSAGE
```

Example output:
```
[GGML_VIZ] [2025-07-10 14:30:09.437] [INFO] Application started
[GGML_VIZ] [2025-07-10 14:30:09.438] [WARN] High memory usage detected
[GGML_VIZ] [2025-07-10 14:30:09.439] [ERROR] Failed to connect to server
```

## Thread Safety

The logger is fully thread-safe and can be used from multiple threads simultaneously. Each log message is atomic and properly synchronized.

## Performance Considerations

- Log level filtering is performed before string formatting
- Disabled log levels have minimal performance impact
- Use formatted logging (`GGML_VIZ_LOG_*_FMT`) for complex messages
- Stream-style logging creates temporary objects but is convenient for complex formatting

## Configuration Precedence Examples

The configuration system respects the following precedence order (highest to lowest):

### Example 1: CLI Override
```bash
# config.json has "level": "ERROR"
# But CLI flag overrides it
./bin/ggml-viz --config config.json --verbose  # Results in DEBUG level
```

### Example 2: Config File Override  
```bash
# Environment has GGML_VIZ_LOG_LEVEL=ERROR
export GGML_VIZ_LOG_LEVEL=ERROR

# But config file overrides environment
./bin/ggml-viz --config config.json  # Uses level from config.json
```

### Example 3: Environment Override
```bash
# No config file specified
# Environment variables are used
export GGML_VIZ_LOG_LEVEL=DEBUG
export GGML_VIZ_LOG_PREFIX="[MY_APP]"
./bin/ggml-viz  # Uses environment settings
```

### Example 4: Demo Configurations
The built-in demos showcase different logging configurations:

```bash
# LLaMA demo uses INFO level with timestamps
./bin/run_llama_vis
# Uses examples/llama_demo/llama_demo_config.json

# Whisper demo uses DEBUG level with thread IDs
./bin/run_whisper_vis  
# Uses examples/whisper_demo/whisper_demo_config.json
```

## Testing

Run the logger test suite to verify functionality:
```bash
./bin/test_logger
```

The test suite validates:
- Basic logging functionality
- Log level filtering
- Formatted logging
- Stream-style logging
- Configuration options
- Environment variable handling
- ConfigManager integration