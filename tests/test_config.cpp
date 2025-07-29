// tests/test_config.cpp
#include "utils/config.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

using namespace ggml_viz;

// Test helper functions
void test_default_config() {
    std::cout << "Testing default configuration..." << std::endl;
    
    Config config = Config::default_config();
    
    // Verify defaults
    assert(config.schema_version == ConfigVersion::V1);
    assert(config.instrumentation.enable_op_timing == true);
    assert(config.instrumentation.enable_memory_tracking == false);
    assert(config.instrumentation.record_tensor_names == true);
    assert(config.instrumentation.max_events == 1000000);
    assert(config.instrumentation.op_types_to_trace.empty());
    assert(config.output.filename == "ggml_trace.ggmlviz");
    assert(config.output.write_to_file == true);
    assert(config.output.flush_interval == 4096);
    assert(config.logging.level == ConfigLogLevel::INFO);
    assert(config.logging.timestamp == true);
    assert(config.logging.thread_id == false);
    assert(config.logging.prefix == "[GGML_VIZ]");
    assert(config.ui.live_mode == false);
    assert(config.ui.poll_interval_ms == 100);
    assert(config.ui.max_live_events == 50000);
    
    // Should be valid
    assert(config.is_valid());
    assert(config.validation_error().empty());
    
    std::cout << "✅ Default config test passed" << std::endl;
}

void test_json_round_trip() {
    std::cout << "Testing JSON serialization round-trip..." << std::endl;
    
    Config original = Config::default_config();
    
    // Modify some values
    original.instrumentation.max_events = 500000;
    original.output.filename = "custom_trace.ggmlviz";
    original.logging.level = ConfigLogLevel::DEBUG;
    original.ui.live_mode = true;
    
    // Serialize to JSON
    std::string json_str = original.to_json();
    assert(!json_str.empty());
    assert(json_str.find("\"schema_version\": 1") != std::string::npos);
    assert(json_str.find("\"max_events\": 500000") != std::string::npos);
    assert(json_str.find("\"filename\": \"custom_trace.ggmlviz\"") != std::string::npos);
    
    // Deserialize back
    Config deserialized = Config::from_json(json_str);
    
    // Verify values match
    assert(deserialized.schema_version == original.schema_version);
    assert(deserialized.instrumentation.max_events == 500000);
    assert(deserialized.output.filename == "custom_trace.ggmlviz");
    assert(deserialized.logging.level == ConfigLogLevel::DEBUG);
    assert(deserialized.ui.live_mode == true);
    
    std::cout << "✅ JSON round-trip test passed" << std::endl;
}

void test_validation() {
    std::cout << "Testing configuration validation..." << std::endl;
    
    // Test invalid max_events
    Config invalid_config = Config::default_config();
    invalid_config.instrumentation.max_events = 0;
    assert(!invalid_config.is_valid());
    assert(invalid_config.validation_error().find("max_events must be greater than 0") != std::string::npos);
    
    // Test empty filename
    Config invalid_config2 = Config::default_config();
    invalid_config2.output.filename = "";
    assert(!invalid_config2.is_valid());
    assert(invalid_config2.validation_error().find("filename cannot be empty") != std::string::npos);
    
    // Test invalid flush_interval
    Config invalid_config3 = Config::default_config();
    invalid_config3.output.flush_interval = 0;
    assert(!invalid_config3.is_valid());
    assert(invalid_config3.validation_error().find("flush_interval must be greater than 0") != std::string::npos);
    
    // Test max_events upper limit
    Config invalid_config4 = Config::default_config();
    invalid_config4.instrumentation.max_events = 200000000; // Over 100M limit
    assert(!invalid_config4.is_valid());
    assert(invalid_config4.validation_error().find("exceeds maximum") != std::string::npos);
    
    std::cout << "✅ Validation test passed" << std::endl;
}

void test_json_parsing_errors() {
    std::cout << "Testing JSON parsing error handling..." << std::endl;
    
    // Test malformed JSON
    try {
        Config::from_json("{invalid json");
        assert(false && "Should have thrown exception");
    } catch (const std::exception& e) {
        assert(std::string(e.what()).find("JSON parsing error") != std::string::npos);
    }
    
    // Test unsupported schema version
    try {
        Config::from_json("{\"schema_version\": 999}");
        assert(false && "Should have thrown exception");
    } catch (const std::exception& e) {
        assert(std::string(e.what()).find("Unsupported schema_version") != std::string::npos);
    }
    
    // Test config that becomes invalid after parsing
    try {
        Config::from_json("{\"schema_version\": 1, \"instrumentation\": {\"max_events\": 0}}");
        assert(false && "Should have thrown exception");
    } catch (const std::exception& e) {
        assert(std::string(e.what()).find("Invalid configuration") != std::string::npos);
    }
    
    std::cout << "✅ JSON parsing error test passed" << std::endl;
}

void test_partial_config_loading() {
    std::cout << "Testing partial configuration loading..." << std::endl;
    
    // Test loading only some fields - others should remain at defaults
    std::string partial_json = R"({
        "schema_version": 1,
        "instrumentation": {
            "max_events": 123456
        },
        "output": {
            "filename": "partial_test.ggmlviz"
        }
    })";
    
    Config config = Config::from_json(partial_json);
    
    // Specified values should be loaded
    assert(config.instrumentation.max_events == 123456);
    assert(config.output.filename == "partial_test.ggmlviz");
    
    // Unspecified values should remain at defaults
    assert(config.instrumentation.enable_op_timing == true);  // default
    assert(config.output.flush_interval == 4096);  // default
    assert(config.logging.level == ConfigLogLevel::INFO);  // default
    
    std::cout << "✅ Partial config loading test passed" << std::endl;
}

void test_config_merging() {
    std::cout << "Testing configuration merging..." << std::endl;
    
    Config base = Config::default_config();
    Config override = Config::default_config();
    
    // Modify override config
    override.instrumentation.max_events = 777777;
    override.output.filename = "merged_trace.ggmlviz";
    override.logging.level = ConfigLogLevel::WARN;
    
    // Merge
    base.merge_from(override);
    
    // Check merged values
    assert(base.instrumentation.max_events == 777777);
    assert(base.output.filename == "merged_trace.ggmlviz");
    assert(base.logging.level == ConfigLogLevel::WARN);
    
    // Other values should remain unchanged
    assert(base.instrumentation.enable_op_timing == true);
    assert(base.output.flush_interval == 4096);
    
    std::cout << "✅ Config merging test passed" << std::endl;
}

void test_env_overrides() {
    std::cout << "Testing environment variable overrides..." << std::endl;
    
    // Set some env vars
    putenv(const_cast<char*>("GGML_VIZ_OUTPUT=env_test.ggmlviz"));
    putenv(const_cast<char*>("GGML_VIZ_MAX_EVENTS=999999"));
    putenv(const_cast<char*>("GGML_VIZ_VERBOSE=1"));
    
    Config config = Config::default_config();
    config.apply_env_overrides();
    
    // Check that env vars were applied
    assert(config.output.filename == "env_test.ggmlviz");
    assert(config.output.write_to_file == true);  // Should be set when OUTPUT is set
    assert(config.instrumentation.max_events == 999999);
    assert(config.logging.level == ConfigLogLevel::DEBUG);  // verbose=1 -> DEBUG
    
    // Test DISABLE flag
    putenv(const_cast<char*>("GGML_VIZ_DISABLE=1"));
    Config disabled_config = Config::default_config();
    disabled_config.apply_env_overrides();
    
    assert(disabled_config.instrumentation.enable_op_timing == false);
    assert(disabled_config.instrumentation.enable_memory_tracking == false);
    assert(disabled_config.output.write_to_file == false);
    
    // Cleanup
    unsetenv("GGML_VIZ_OUTPUT");
    unsetenv("GGML_VIZ_MAX_EVENTS");
    unsetenv("GGML_VIZ_VERBOSE");
    unsetenv("GGML_VIZ_DISABLE");
    
    std::cout << "✅ Environment override test passed" << std::endl;
}

void test_config_manager_singleton() {
    std::cout << "Testing ConfigManager singleton..." << std::endl;
    
    ConfigManager& mgr1 = ConfigManager::instance();
    ConfigManager& mgr2 = ConfigManager::instance();
    
    // Should be the same instance
    assert(&mgr1 == &mgr2);
    
    // Initially not loaded
    assert(!mgr1.is_loaded());
    
    // Test default dump before loading
    std::string default_dump = mgr1.dump_json();
    assert(!default_dump.empty());
    assert(default_dump.find("\"schema_version\": 1") != std::string::npos);
    
    std::cout << "✅ ConfigManager singleton test passed" << std::endl;
}

void test_file_loading() {
    std::cout << "Testing file loading..." << std::endl;
    
    // Create a test config file
    std::string test_config = R"({
        "schema_version": 1,
        "instrumentation": {
            "max_events": 654321,
            "record_tensor_names": false
        },
        "output": {
            "filename": "file_test.ggmlviz",
            "flush_interval": 2048
        },
        "logging": {
            "level": "ERROR",
            "prefix": "[TEST]"
        }
    })";
    
    std::ofstream file("test_config.json");
    file << test_config;
    file.close();
    
    // Load from file
    Config config = Config::from_file("test_config.json");
    
    // Verify loaded values
    assert(config.instrumentation.max_events == 654321);
    assert(config.instrumentation.record_tensor_names == false);
    assert(config.output.filename == "file_test.ggmlviz");
    assert(config.output.flush_interval == 2048);
    assert(config.logging.level == ConfigLogLevel::ERROR_LEVEL);
    assert(config.logging.prefix == "[TEST]");
    
    // Cleanup
    std::remove("test_config.json");
    
    std::cout << "✅ File loading test passed" << std::endl;
}

void test_precedence_loading() {
    std::cout << "Testing precedence loading..." << std::endl;
    
    ConfigManager& mgr = ConfigManager::instance();
    mgr.reset();
    
    // Create test config files
    std::string default_config = R"({
        "schema_version": 1,
        "instrumentation": {"max_events": 100000},
        "output": {"filename": "default.ggmlviz"}
    })";
    
    std::string cli_config = R"({
        "schema_version": 1,
        "instrumentation": {"max_events": 200000},
        "output": {"filename": "cli.ggmlviz"}
    })";
    
    std::ofstream default_file("default_config.json");
    default_file << default_config;
    default_file.close();
    
    std::ofstream cli_file("cli_config.json");
    cli_file << cli_config;
    cli_file.close();
    
    // Load with precedence (CLI should win)
    mgr.load_with_precedence("cli_config.json", "", "default_config.json");
    
    const Config& loaded_config = mgr.get();
    
    // CLI config should take precedence
    assert(loaded_config.instrumentation.max_events == 200000);
    assert(loaded_config.output.filename == "cli.ggmlviz");
    assert(mgr.is_loaded());
    
    // Test dump
    std::string dumped = mgr.dump_json();
    assert(dumped.find("\"max_events\": 200000") != std::string::npos);
    
    // Cleanup
    std::remove("default_config.json");
    std::remove("cli_config.json");
    
    std::cout << "✅ Precedence loading test passed" << std::endl;
}

void test_unknown_keys_warning() {
    std::cout << "Testing unknown keys warning..." << std::endl;
    
    std::string config_with_unknown = R"({
        "schema_version": 1,
        "instrumentation": {
            "max_events": 123456
        },
        "unknown_section": {
            "some_field": "value"
        },
        "another_unknown": 42
    })";
    
    // Should parse successfully but warn about unknown keys
    // (We can't easily test stderr output in this test, but the config should load)
    Config config = Config::from_json(config_with_unknown);
    assert(config.is_valid());
    assert(config.instrumentation.max_events == 123456);
    
    std::cout << "✅ Unknown keys warning test passed" << std::endl;
}

// Golden file test for --dump-config round-trip
void test_dump_config_golden() {
    std::cout << "Testing --dump-config golden file round-trip..." << std::endl;
    
    Config original = Config::default_config();
    
    // Serialize to JSON
    std::string json1 = original.to_json();
    
    // Parse it back
    Config parsed = Config::from_json(json1);
    
    // Serialize again
    std::string json2 = parsed.to_json();
    
    // Should be identical
    assert(json1 == json2);
    
    // Both should parse to identical configs
    assert(original.instrumentation.max_events == parsed.instrumentation.max_events);
    assert(original.output.filename == parsed.output.filename);
    assert(original.logging.level == parsed.logging.level);
    
    std::cout << "✅ Dump config golden test passed" << std::endl;
}

int main() {
    std::cout << "Running configuration system tests..." << std::endl;
    
    try {
        test_default_config();
        test_json_round_trip();
        test_validation();
        test_json_parsing_errors();
        test_partial_config_loading();
        test_config_merging();
        test_env_overrides();
        test_config_manager_singleton();
        test_file_loading();
        test_precedence_loading();
        test_unknown_keys_warning();
        test_dump_config_golden();
        
        std::cout << "\n🎉 All configuration tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}