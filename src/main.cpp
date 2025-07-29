// src/main.cpp
#include "frontend/imgui_app.hpp"
#include "server/live_data_collector.hpp"
#include "utils/logger.hpp"
#include "utils/config.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#ifndef _WIN32
#include <getopt.h>
#endif
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>
#include <atomic>

namespace {
    const char* VERSION = "0.0.7";
    const char* PROGRAM_NAME = "ggml-viz";
    
    struct Config {
        std::string trace_file;
        std::string config_file;
        bool live_mode = false;
        bool web_mode = false;
        bool verbose = false;
        bool show_help = false;
        bool show_version = false;
        bool no_hook = false;
        int port = 8080;
    };

#ifdef _WIN32
    // Windows doesn't have getopt.h, we'll implement simple option parsing
    void parse_windows_args(int argc, char* argv[], Config& config) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                config.show_help = true;
            } else if (arg == "-V" || arg == "--version") {
                config.show_version = true;
            } else if (arg == "-v" || arg == "--verbose") {
                config.verbose = true;
            } else if (arg == "-l" || arg == "--live") {
                config.live_mode = true;
            } else if (arg == "-w" || arg == "--web") {
                config.web_mode = true;
            } else if (arg == "-p" || arg == "--port") {
                if (i + 1 < argc) {
                    config.port = std::atoi(argv[++i]);
                }
            } else if (arg == "-c" || arg == "--config") {
                if (i + 1 < argc) {
                    config.config_file = argv[++i];
                }
            } else if (arg == "--no-hook") {
                config.no_hook = true;
            } else if (arg[0] != '-') {
                config.trace_file = arg;
            }
        }
    }
#endif
    
    void print_usage(const char* program_name) {
        std::cout << "Usage: " << program_name << " [OPTIONS] [TRACE_FILE]\n\n"
                  << "GGML Visualizer - Real-time dashboard for GGML-based LLM runtimes\n\n"
                  << "Arguments:\n"
                  << "  TRACE_FILE              Path to .ggmlviz trace file to load\n\n"
                  << "Options:\n"
                  << "  -h, --help              Show this help message and exit\n"
                  << "  -V, --version           Show version information and exit\n"
                  << "  -v, --verbose           Enable verbose logging output\n"
                  << "  -l, --live              Enable live mode (real-time GUI updates)\n"
                  << "  -w, --web               Enable web server mode (browser interface)\n"
                  << "  -p, --port PORT         Port for web server (default: 8080)\n"
                  << "  -c, --config FILE       Load configuration from file\n"
                  << "      --no-hook           Disable built-in hook (for external hook usage)\n\n"
                  << "Examples:\n"
                  << "  " << program_name << "                          # Open empty dashboard\n"
                  << "  " << program_name << " trace.ggmlviz           # Load specific trace file\n"
                  << "  " << program_name << " --live                   # Live mode with GUI\n"
                  << "  " << program_name << " --web --port 9000       # Web server on port 9000\n"
                  << "  " << program_name << " --verbose trace.ggmlviz # Load with verbose output\n\n"
                  << "Environment Variables:\n"
                  << "  Essential Variables:\n"
                  << "    GGML_VIZ_OUTPUT       Output file for trace recording\n"
                  << "    GGML_VIZ_VERBOSE      Enable verbose instrumentation logging\n"
                  << "    GGML_VIZ_DISABLE      Disable instrumentation entirely\n"
                  << "\n"
                  << "  Library Injection:\n"
                  << "    DYLD_INSERT_LIBRARIES Path to libggml_viz_hook.dylib (macOS)\n"
                  << "    LD_PRELOAD            Path to libggml_viz_hook.so (Linux)\n"
                  << "\n"
                  << "  Configuration Variables:\n"
                  << "    GGML_VIZ_MAX_EVENTS   Maximum events to capture (default: 10,000,000)\n"
                  << "    GGML_VIZ_OP_TIMING    Enable operation timing (default: true)\n"
                  << "    GGML_VIZ_MEMORY_TRACKING  Enable memory tracking (default: false)\n"
                  << "    GGML_VIZ_THREAD_TRACKING  Enable thread tracking (default: false)\n"
                  << "    GGML_VIZ_TENSOR_NAMES     Capture tensor names (default: true)\n"
                  << "\n"
                  << "  Logging Configuration:\n"
                  << "    GGML_VIZ_LOG_LEVEL    Log level (DEBUG/INFO/WARN/ERROR/FATAL)\n"
                  << "    GGML_VIZ_LOG_TIMESTAMP    Enable timestamps (default: true)\n"
                  << "    GGML_VIZ_LOG_THREAD_ID    Enable thread IDs (default: false)\n"
                  << "    GGML_VIZ_LOG_PREFIX   Custom log prefix (default: [GGML_VIZ])\n\n"
                  << "For more information, visit: https://github.com/brodheadw/ggml-viz\n";
    }
    
    void print_version() {
        std::cout << PROGRAM_NAME << " version " << VERSION << "\n"
                  << "GGML Visualizer - Real-time dashboard for GGML-based LLM runtimes\n"
                  << "Built with ImGui and GGML instrumentation hooks\n\n"
                  << "Copyright 2024. Licensed under Apache 2.0.\n";
    }
    
    Config parse_arguments(int argc, char* argv[]) {
        Config config;
        
        // Define long options
        
#ifdef _WIN32
        // Use Windows argument parsing
        parse_windows_args(argc, argv, config);
#else
        static struct option long_options[] = {
            {"help",    no_argument,       0, 'h'},
            {"version", no_argument,       0, 'V'},
            {"verbose", no_argument,       0, 'v'},
            {"live",    no_argument,       0, 'l'},
            {"web",     no_argument,       0, 'w'},
            {"port",    required_argument, 0, 'p'},
            {"config",  required_argument, 0, 'c'},
            {"no-hook", no_argument,       0, 1000},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c;
        
        // Parse options
        while ((c = getopt_long(argc, argv, "hVvlwp:c:", long_options, &option_index)) != -1) {
            switch (c) {
                case 'h':
                    config.show_help = true;
                    break;
                case 'V':
                    config.show_version = true;
                    break;
                case 'v':
                    config.verbose = true;
                    break;
                case 'l':
                    config.live_mode = true;
                    break;
                case 'w':
                    config.web_mode = true;
                    break;
                case 'p':
                    try {
                        config.port = std::stoi(optarg);
                        if (config.port < 1 || config.port > 65535) {
                            throw std::out_of_range("Port out of range");
                        }
                    } catch (const std::exception&) {
                        GGML_VIZ_LOG_ERROR_FMT("Invalid port number: %s", optarg);
                        GGML_VIZ_LOG_ERROR("Port must be between 1 and 65535");
                        exit(1);
                    }
                    break;
                case 'c':
                    config.config_file = optarg;
                    break;
                case 1000:  // --no-hook
                    config.no_hook = true;
                    break;
                case '?':
                    // getopt_long already printed an error message
                    exit(1);
                default:
                    std::cerr << "Error: Unknown option\n";
                    exit(1);
            }
        }
        
        // Parse positional arguments (trace file)
        if (optind < argc) {
            config.trace_file = argv[optind];
            
            // Check if there are extra arguments
            if (optind + 1 < argc) {
                std::cerr << "Error: Too many arguments. Only one trace file can be specified.\n";
                std::cerr << "Use --help for usage information.\n";
                exit(1);
            }
        }
#endif
        
        return config;
    }
    
    void validate_config(const Config& config) {
        // Validate trace file exists if specified
        if (!config.trace_file.empty()) {
            std::ifstream file(config.trace_file);
            if (!file.good()) {
                std::cerr << "Error: Cannot open trace file: " << config.trace_file << "\n";
                exit(1);
            }
        }
        
        // Validate config file exists if specified
        if (!config.config_file.empty()) {
            std::ifstream file(config.config_file);
            if (!file.good()) {
                std::cerr << "Error: Cannot open config file: " << config.config_file << "\n";
                exit(1);
            }
        }
        
        // Live mode validation
        if (config.live_mode && !config.trace_file.empty() && !config.no_hook) {
            std::cerr << "Error: Cannot specify trace file in live mode without --no-hook.\n";
            std::cerr << "       Live mode captures data in real-time.\n";
            std::cerr << "       Use --no-hook to monitor an external trace file.\n";
            exit(1);
        }
        
        // Config file validation is handled during loading
        // No need to check existence here as ConfigManager handles fallbacks
    }
    
    void setup_environment(const Config& config) {
        // Load configuration through ConfigManager with CLI precedence
        auto& config_mgr = ggml_viz::ConfigManager::instance();
        
        // Set environment variables for backward compatibility
        if (config.verbose) {
#ifdef _WIN32
            _putenv_s("GGML_VIZ_VERBOSE", "1");
#else
            setenv("GGML_VIZ_VERBOSE", "1", 1);
#endif
        }
        
        // Load configuration with CLI precedence: CLI > env file > defaults
        const char* env_config_file = getenv("GGML_VIZ_CONFIG");
        config_mgr.load_with_precedence(
            config.config_file,                    // CLI --config flag (highest precedence)
            env_config_file ? env_config_file : "", // GGML_VIZ_CONFIG env var
            "ggml-viz.json"                        // Default config file
        );
        
        // Configure logger from the loaded configuration
        auto app_config = config_mgr.get();
        ggml_viz::Logger::instance().configure_from_config(*app_config);
        
        // Print configuration if verbose
        if (config.verbose) {
            GGML_VIZ_LOG_INFO("CLI Configuration:");
            GGML_VIZ_LOG_INFO_FMT("  Trace file: %s", config.trace_file.empty() ? "(none)" : config.trace_file.c_str());
            GGML_VIZ_LOG_INFO_FMT("  Config file: %s", config.config_file.empty() ? "(none)" : config.config_file.c_str());
            GGML_VIZ_LOG_INFO_FMT("  Live mode: %s", config.live_mode ? "enabled" : "disabled");
            GGML_VIZ_LOG_INFO_FMT("  Port: %d", config.port);
            GGML_VIZ_LOG_INFO_FMT("  Verbose: %s", config.verbose ? "enabled" : "disabled");
            
            GGML_VIZ_LOG_INFO("\nLoaded GGML Configuration:");
            GGML_VIZ_LOG_INFO_FMT("  Output file: %s", app_config->output.filename.c_str());
            GGML_VIZ_LOG_INFO_FMT("  Max events: %llu", app_config->instrumentation.max_events);
            GGML_VIZ_LOG_INFO_FMT("  Op timing: %s", app_config->instrumentation.enable_op_timing ? "enabled" : "disabled");
            GGML_VIZ_LOG_INFO_FMT("  Memory tracking: %s", app_config->instrumentation.enable_memory_tracking ? "enabled" : "disabled");
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        Config config = parse_arguments(argc, argv);
        
        // Handle help and version options
        if (config.show_help) {
            print_usage(argv[0]);
            return 0;
        }
        
        if (config.show_version) {
            print_version();
            return 0;
        }
        
        // Validate configuration
        validate_config(config);
        
        // Setup environment based on config
        setup_environment(config);
        
        // Create and configure the application
        ggml_viz::ImGuiApp app;
        
        // Handle different modes
        if (config.web_mode) {
            // Web server mode: Start HTTP server for browser interface
            if (config.verbose) {
                std::cout << "Starting web server mode on port " << config.port << "\n";
            }
            
            // Create live stream server
            ggml_viz::LiveStreamServer::StreamConfig stream_config;
            stream_config.port = config.port;
            stream_config.host = "localhost";
            
            auto live_server = std::make_unique<ggml_viz::LiveStreamServer>(stream_config);
            
            // Setup signal handler for graceful shutdown
            std::atomic<bool> shutdown_requested{false};
            signal(SIGINT, [](int) { 
                printf("\nShutdown requested...\n");
                exit(0);
            });
            
            // Start the live server
            live_server->start();
            
            std::cout << "GGML Visualizer Web Server Started\n";
            std::cout << "==================================\n";
            std::cout << "🌐 Web Dashboard: http://localhost:" << config.port << "\n";
            std::cout << "📡 Event Stream:  http://localhost:" << config.port << "/events\n";
            std::cout << "📊 Status API:    http://localhost:" << config.port << "/status\n";
            std::cout << "Press Ctrl+C to stop\n\n";
            
            // Keep server running until interrupted
            while (live_server->is_running()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                if (config.verbose && live_server->client_count() > 0) {
                    std::cout << "Active clients: " << live_server->client_count() << "\n";
                }
            }
            
            live_server->stop();
            return 0;
            
        } else if (config.live_mode) {
            // Live GUI mode: Enable real-time updates in ImGui
            if (config.verbose) {
                std::cout << "Starting live mode GUI...\n";
                std::cout << "The GUI will automatically update with live GGML events.\n";
                std::cout << "Run GGML applications with GGML_VIZ_OUTPUT=trace.ggmlviz\n";
            }
            
            app.enable_live_mode(config.no_hook, config.trace_file);
            std::cout << "GGML Visualizer Live Mode (GUI)\n";
            std::cout << "===============================\n";
            std::cout << "✅ Live mode enabled in GUI\n";
            std::cout << "🔧 Set environment: export GGML_VIZ_OUTPUT=trace.ggmlviz\n";
            std::cout << "🦙 Run inference: ollama run model \"prompt\" or llama.cpp\n";
            std::cout << "👁️  Watch real-time events in the GUI timeline and graph views\n\n";
            
        } else {
            // Regular mode: Load trace file
            if (!config.trace_file.empty()) {
                if (config.verbose) {
                    std::cout << "Loading trace file: " << config.trace_file << "\n";
                }
                app.load_trace_file(config.trace_file);
            } else if (config.verbose) {
                std::cout << "Starting with empty dashboard.\n";
            }
        }
        
        // Run the GUI application
        return app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error: An unexpected error occurred." << std::endl;
        return 1;
    }
}