// src/frontend/imgui_app.cpp
#include "imgui_app.hpp"
#include "utils/trace_reader.hpp"
#include "instrumentation/ggml_hook.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <chrono>
#include <ctime>
#include <sys/stat.h>
#include <unordered_map>

namespace ggml_viz {

struct ImGuiApp::AppData {
    GLFWwindow* window = nullptr;
    std::unique_ptr<TraceReader> trace_reader;
    std::string current_filename;
    std::string error_message;
    
    // UI state
    bool trace_loaded = false;
    int selected_event = -1;
    
    // Live mode support
    bool live_mode = false;
    std::vector<Event> live_events;
    
    // Cached live memory stats for performance
    size_t live_memory_last_processed_event = 0;
    size_t live_total_allocs = 0, live_total_frees = 0;
    size_t live_bytes_allocated = 0, live_current_usage = 0, live_peak_usage = 0;
    std::unordered_map<const void*, size_t> live_allocations;
    std::chrono::steady_clock::time_point last_live_update;
    std::atomic<bool> live_data_available{false};
    
    // File monitoring for external processes
    std::string live_file_path;
    std::time_t last_file_mod_time = 0;
    size_t last_file_size = 0;
    std::unique_ptr<TraceReader> live_trace_reader;
    
    // File browser state
    char file_path_buffer[512] = {0};
};

ImGuiApp::ImGuiApp() : data_(std::make_unique<AppData>()) {
}

ImGuiApp::~ImGuiApp() {
    shutdown();
}

int ImGuiApp::run() {
    if (!initialize()) {
        return -1;
    }
    
    // Main loop
    while (!glfwWindowShouldClose(data_->window)) {
        glfwPollEvents();
        render_frame();
    }
    
    shutdown();
    return 0;
}

bool ImGuiApp::initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    
    // Set GLFW window hints for OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // Create window
    data_->window = glfwCreateWindow(1280, 720, "GGML Visualizer", nullptr, nullptr);
    if (!data_->window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(data_->window);
    glfwSwapInterval(1); // Enable vsync
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(data_->window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    
    return true;
}

void ImGuiApp::enable_live_mode(bool no_hook, const std::string& trace_file) {
    data_->live_mode = true;
    data_->live_events.clear();
    data_->last_live_update = std::chrono::steady_clock::now();
    data_->current_filename = "[Live Mode]";
    
    // Initialize and start the GGML hook (unless disabled)
    if (!no_hook) {
        try {
            auto& hook = GGMLHook::instance();
            
            // Configure the hook
            HookConfig config;
            config.enable_op_timing = true;
            config.enable_memory_tracking = true;  // Enable memory tracking for live visualization
            config.enable_thread_tracking = false;
            config.enable_tensor_names = true;
            config.write_to_file = false;  // Don't write to file in live mode
            config.max_events = 100000;   // Buffer for live events
            
            hook.configure(config);
            hook.start();
            
            std::cout << "[ImGuiApp] Live mode enabled and GGML hook started" << std::endl;
            std::cout << "[ImGuiApp] Hook active: " << (hook.is_active() ? "YES" : "NO") << std::endl;
            std::cout << "[ImGuiApp] DEBUG: Configuration applied:" << std::endl;
            std::cout << "[ImGuiApp]   - Op timing: " << (config.enable_op_timing ? "ENABLED" : "DISABLED") << std::endl;
            std::cout << "[ImGuiApp]   - Memory tracking: " << (config.enable_memory_tracking ? "ENABLED" : "DISABLED") << std::endl;
            std::cout << "[ImGuiApp]   - Thread tracking: " << (config.enable_thread_tracking ? "ENABLED" : "DISABLED") << std::endl;
            std::cout << "[ImGuiApp]   - Tensor names: " << (config.enable_tensor_names ? "ENABLED" : "DISABLED") << std::endl;
            std::cout << "[ImGuiApp]   - Write to file: " << (config.write_to_file ? "ENABLED" : "DISABLED") << std::endl;
            std::cout << "[ImGuiApp]   - Max events: " << config.max_events << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[ImGuiApp] Error starting GGML hook: " << e.what() << std::endl;
        }
    } else {
        std::cout << "[ImGuiApp] Live mode enabled with built-in hook disabled (--no-hook)" << std::endl;
    }
    
    // Set up file monitoring for external processes
    // Priority: 1) command line argument, 2) environment variable, 3) default
    if (!trace_file.empty()) {
        data_->live_file_path = trace_file;
        std::cout << "[ImGuiApp] Monitoring specified trace file: " << data_->live_file_path << std::endl;
    } else {
        const char* env_output = std::getenv("GGML_VIZ_OUTPUT");
        if (env_output) {
            data_->live_file_path = env_output;
            std::cout << "[ImGuiApp] Monitoring external trace file: " << data_->live_file_path << std::endl;
        } else {
            // Default to monitoring a temporary file
            data_->live_file_path = "test.ggmlviz";
            std::cout << "[ImGuiApp] No GGML_VIZ_OUTPUT set, monitoring default: " << data_->live_file_path << std::endl;
        }
    }
    
    // Initialize file monitoring state
    data_->last_file_mod_time = 0;
    data_->last_file_size = 0;
}

void ImGuiApp::disable_live_mode() {
    data_->live_mode = false;
    data_->live_events.clear();
    
    // Stop the GGML hook (if it was started)
    try {
        auto& hook = GGMLHook::instance();
        if (hook.is_active()) {
            hook.stop();
            std::cout << "[ImGuiApp] Live mode disabled and GGML hook stopped" << std::endl;
        } else {
            std::cout << "[ImGuiApp] Live mode disabled (hook was not active)" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ImGuiApp] Error stopping GGML hook: " << e.what() << std::endl;
    }
    
    // Clean up file monitoring
    data_->live_file_path.clear();
    data_->last_file_mod_time = 0;
    data_->last_file_size = 0;
    data_->live_trace_reader.reset();
}

bool ImGuiApp::is_live_mode() const {
    return data_->live_mode;
}

void ImGuiApp::update_live_data() {
    if (!data_->live_mode) return;
    
    static int call_count = 0;
    if (++call_count % 100 == 0) {  // Only print every 100 calls to avoid spam
        std::cout << "[ImGuiApp] update_live_data() called (file: " << data_->live_file_path 
                  << ", events: " << data_->live_events.size() << ")" << std::endl;
    }
    
    // Try to get live events from GGMLHook (in-process events)
    try {
        auto& hook = GGMLHook::instance();
        if (hook.is_active()) {
            auto new_events = hook.consume_available_events();
            if (!new_events.empty()) {
                // DEBUG: Count event types in new batch
                size_t new_memory_events = 0;
                for (const auto& event : new_events) {
                    if (event.type == EventType::TENSOR_ALLOC || event.type == EventType::TENSOR_FREE) {
                        new_memory_events++;
                    }
                }
                
                if (call_count % 100 == 0) {  // Only print every 100 calls
                    std::cout << "[ImGuiApp] DEBUG: Got " << new_events.size() << " new events (" 
                              << new_memory_events << " memory events)" << std::endl;
                }
                
                // Add new events to our live buffer
                data_->live_events.insert(data_->live_events.end(), new_events.begin(), new_events.end());
                data_->last_live_update = std::chrono::steady_clock::now();
                data_->live_data_available = true;
                
                // Limit buffer size to prevent memory issues
                const size_t max_events = 50000;
                if (data_->live_events.size() > max_events) {
                    data_->live_events.erase(data_->live_events.begin(), 
                                           data_->live_events.begin() + (data_->live_events.size() - max_events));
                }
            }
        } else {
            if (call_count % 100 == 0) {
                std::cout << "[ImGuiApp] DEBUG: Hook is not active" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ImGuiApp] Error updating live data from hook: " << e.what() << std::endl;
    }
    
    // Also monitor external trace file for events from external processes
    // Only check file every 100ms to avoid too frequent polling
    static auto last_file_check = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_check = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_file_check);
    
    if (!data_->live_file_path.empty() && time_since_last_check.count() > 100) {
        last_file_check = now;
        
        try {
            struct stat file_stat;
            if (stat(data_->live_file_path.c_str(), &file_stat) == 0) {
                // Check if file has been modified
                if (file_stat.st_mtime > data_->last_file_mod_time || 
                    static_cast<size_t>(file_stat.st_size) > data_->last_file_size) {
                    
                    // File has been updated, reload it
                    std::cout << "[ImGuiApp] File changed - reloading: " << data_->live_file_path 
                              << " (size: " << file_stat.st_size << " bytes)" << std::endl;
                    auto new_trace_reader = std::make_unique<TraceReader>(data_->live_file_path);
                    if (new_trace_reader->is_valid()) {
                        const auto& events = new_trace_reader->events();
                        
                        // Handle file recreation/truncation by detecting if file has fewer events than expected
                        static size_t last_file_event_count = 0;
                        size_t start_idx = 0;
                        
                        if (data_->live_trace_reader && events.size() >= last_file_event_count) {
                            // File appears to be growing normally, only load new events
                            start_idx = last_file_event_count;
                        } else {
                            // File was recreated/truncated or this is first load, load all events
                            start_idx = 0;
                            std::cout << "[ImGuiApp] File appears to be recreated/truncated, loading all events" << std::endl;
                        }
                        
                        if (events.size() > start_idx) {
                            // Add new events to our live buffer
                            size_t new_event_count = events.size() - start_idx;
                            data_->live_events.insert(data_->live_events.end(), 
                                                     events.begin() + start_idx, events.end());
                            data_->last_live_update = std::chrono::steady_clock::now();
                            data_->live_data_available = true;
                            
                            std::cout << "[ImGuiApp] Loaded " << new_event_count 
                                      << " new events from external file (total events in file: " << events.size() << ")" << std::endl;
                            
                            last_file_event_count = events.size();
                        } else {
                            std::cout << "[ImGuiApp] No new events to load (file has " << events.size() 
                                      << " events, last processed: " << last_file_event_count << ")" << std::endl;
                        }
                        
                        // Update file monitoring state
                        data_->last_file_mod_time = file_stat.st_mtime;
                        data_->last_file_size = file_stat.st_size;
                        data_->live_trace_reader = std::move(new_trace_reader);
                        
                        // Limit buffer size
                        const size_t max_events = 50000;
                        if (data_->live_events.size() > max_events) {
                            data_->live_events.erase(data_->live_events.begin(), 
                                                   data_->live_events.begin() + (data_->live_events.size() - max_events));
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[ImGuiApp] Error monitoring external file: " << e.what() << std::endl;
        }
    }
}

void ImGuiApp::shutdown() {
    if (data_->window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(data_->window);
        glfwTerminate();
        data_->window = nullptr;
    }
}

void ImGuiApp::render_frame() {
    // Update live data if in live mode
    update_live_data();
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Enable docking
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    
    // Render main menu bar
    render_main_menu_bar();
    
    // Render GUI panels
    if (show_file_browser_) {
        render_file_browser();
    }
    
    if ((data_->trace_loaded && data_->trace_reader) || data_->live_mode) {
        if (show_timeline_) {
            render_timeline_view();
        }
        if (show_graph_) {
            render_graph_view();
        }
        if (show_tensor_inspector_) {
            render_tensor_inspector();
        }
        if (show_memory_view_) {
            render_memory_view();
        }
    }
    
    // Show ImGui demo window if requested
    if (show_demo_window_) {
        ImGui::ShowDemoWindow(&show_demo_window_);
    }
    
    // Error popup
    if (!data_->error_message.empty()) {
        ImGui::OpenPopup("Error");
        if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", data_->error_message.c_str());
            if (ImGui::Button("OK")) {
                data_->error_message.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    
    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(data_->window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glfwSwapBuffers(data_->window);
}

void ImGuiApp::render_main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Trace...")) {
                show_file_browser_ = true;
            }
            if (ImGui::MenuItem("Close Trace", nullptr, false, data_->trace_loaded)) {
                data_->trace_reader.reset();
                data_->trace_loaded = false;
                data_->current_filename.clear();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(data_->window, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Timeline", nullptr, &show_timeline_, data_->trace_loaded)) {}
            if (ImGui::MenuItem("Graph", nullptr, &show_graph_, data_->trace_loaded)) {}
            if (ImGui::MenuItem("Tensor Inspector", nullptr, &show_tensor_inspector_, data_->trace_loaded)) {}
            if (ImGui::MenuItem("Memory View", nullptr, &show_memory_view_, data_->trace_loaded)) {}
            ImGui::Separator();
            ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // TODO: Implement about dialog
            }
            ImGui::EndMenu();
        }
        
        // Status in menu bar
        if (data_->trace_loaded) {
            ImGui::SameLine(ImGui::GetWindowWidth() - 300);
            ImGui::Text("Loaded: %s (%zu events)", 
                       data_->current_filename.c_str(), 
                       data_->trace_reader->event_count());
        }
        
        ImGui::EndMainMenuBar();
    }
}

void ImGuiApp::render_file_browser() {
    if (ImGui::Begin("Open Trace File", &show_file_browser_)) {
        ImGui::Text("Enter path to .ggmlviz file:");
        ImGui::InputText("##filepath", data_->file_path_buffer, sizeof(data_->file_path_buffer));
        
        ImGui::Separator();
        
        if (ImGui::Button("Open")) {
            if (strlen(data_->file_path_buffer) > 0) {
                if (load_trace_file(data_->file_path_buffer)) {
                    show_file_browser_ = false;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            show_file_browser_ = false;
        }
        
        // TODO: Add proper file browser with directory listing
        ImGui::Text("Note: Enter full path to trace file");
        ImGui::Text("Example: /path/to/trace.ggmlviz");
    }
    ImGui::End();
}

bool ImGuiApp::load_trace_file(const std::string& filename) {
    try {
        // First check if file exists and is accessible
        FILE* test_file = fopen(filename.c_str(), "rb");
        if (!test_file) {
            // Determine specific reason for file access failure
            if (filename.empty()) {
                data_->error_message = "Error: No file path specified.";
            } else if (filename.find_last_of('.') == std::string::npos || 
                      filename.substr(filename.find_last_of('.')) != ".ggmlviz") {
                data_->error_message = "Error: Invalid file type.\n\nExpected a .ggmlviz trace file.\nSelected: " + filename;
            } else {
                data_->error_message = "Error: File not found or access denied.\n\nFile: " + filename + 
                                      "\n\nPlease check:\n• File exists\n• File permissions\n• Path is correct";
            }
            return false;
        }
        
        // Check if file is empty
        fseek(test_file, 0, SEEK_END);
        long file_size = ftell(test_file);
        fclose(test_file);
        
        if (file_size == 0) {
            data_->error_message = "Error: Empty trace file.\n\nFile: " + filename + 
                                  "\n\nThe trace file contains no data. Please ensure the file was generated correctly.";
            return false;
        }
        
        if (file_size < 12) {  // Minimum size for header
            data_->error_message = "Error: Invalid trace file.\n\nFile: " + filename + 
                                  "\n\nFile is too small (" + std::to_string(file_size) + " bytes) to contain valid trace data.";
            return false;
        }
        
        // Now attempt to load with TraceReader
        auto reader = std::make_unique<TraceReader>(filename);
        if (!reader->is_valid()) {
            // Determine specific reason for TraceReader failure
            FILE* check_file = fopen(filename.c_str(), "rb");
            if (check_file) {
                char magic[8] = {0};
                if (fread(magic, 1, 8, check_file) == 8) {
                    if (strncmp(magic, "GGMLVIZ1", 8) != 0) {
                        data_->error_message = "Error: Invalid trace file format.\n\nFile: " + filename + 
                                              "\n\nThis does not appear to be a valid GGML trace file.\n" +
                                              "Expected magic header 'GGMLVIZ1', found: '" + std::string(magic, 8) + "'";
                    } else {
                        data_->error_message = "Error: Corrupted trace file.\n\nFile: " + filename + 
                                              "\n\nThe file header is valid but the trace data appears to be corrupted.\n" +
                                              "The file may have been truncated or damaged.";
                    }
                } else {
                    data_->error_message = "Error: Cannot read trace file header.\n\nFile: " + filename + 
                                          "\n\nFile exists but cannot be read properly. Check file permissions.";
                }
                fclose(check_file);
            } else {
                data_->error_message = "Error: File access lost during loading.\n\nFile: " + filename;
            }
            return false;
        }
        
        // Check if trace file has any meaningful data
        if (reader->event_count() == 0) {
            data_->error_message = "Warning: Empty trace data.\n\nFile: " + filename + 
                                  "\n\nThe trace file loaded successfully but contains no events.\n" +
                                  "This might indicate:\n• No GGML operations were traced\n• Tracing was not enabled\n• The model ran but no operations occurred";
            // Still allow loading empty traces for debugging
        }
        
        // Success - load the trace
        data_->trace_reader = std::move(reader);
        data_->trace_loaded = true;
        data_->current_filename = filename.substr(filename.find_last_of("/\\") + 1); // Just filename for display
        data_->selected_event = -1;
        
        return true;
        
    } catch (const std::bad_alloc& e) {
        data_->error_message = "Error: Out of memory.\n\nFile: " + filename + 
                              "\n\nNot enough memory to load this trace file.\n" +
                              "Try closing other applications or loading a smaller trace file.";
        return false;
    } catch (const std::exception& e) {
        data_->error_message = "Error: Unexpected error loading trace.\n\nFile: " + filename + 
                              "\n\nDetails: " + std::string(e.what()) + 
                              "\n\nThis may indicate a bug in the application or a severely corrupted file.";
        return false;
    } catch (...) {
        data_->error_message = "Error: Unknown error occurred.\n\nFile: " + filename + 
                              "\n\nAn unexpected error occurred while loading the trace file.";
        return false;
    }
}

void ImGuiApp::render_timeline_view() {
    if (ImGui::Begin("Timeline View")) {
        // Handle both live mode and loaded traces
        const std::vector<Event>* events_ptr = nullptr;
        size_t event_count = 0;
        std::string mode_info;
        
        if (data_->live_mode) {
            events_ptr = &data_->live_events;
            event_count = data_->live_events.size();
            mode_info = "[LIVE MODE]";
            
            // Show live mode status
            auto& hook = GGMLHook::instance();
            ImGui::Text("🔴 LIVE MODE - Hook Active: %s", hook.is_active() ? "YES" : "NO");
            ImGui::Text("Live Events: %zu", event_count);
            
            if (event_count > 0) {
                auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - data_->last_live_update).count();
                ImGui::Text("Last Update: %lld ms ago", time_since_last);
            }
            
        } else if (data_->trace_reader) {
            events_ptr = &data_->trace_reader->events();
            event_count = data_->trace_reader->events().size();
            mode_info = "[LOADED TRACE]";
        } else {
            ImGui::Text("No trace loaded and live mode not active");
            ImGui::End();
            return;
        }
        
        const auto& events = *events_ptr;
        
        // Summary information
        ImGui::Text("%s", mode_info.c_str());
        ImGui::Text("Total Events: %zu", events.size());
        
        if (data_->live_mode) {
            // Calculate duration from live events
            if (events.size() >= 2) {
                uint64_t duration_ns = events.back().timestamp_ns - events.front().timestamp_ns;
                ImGui::Text("Duration: %.2f ms", duration_ns / 1e6);
            } else {
                ImGui::Text("Duration: N/A");
            }
            ImGui::Text("Operations: Live counting...");
        } else if (data_->trace_reader) {
            auto op_timings = data_->trace_reader->get_op_timings();
            ImGui::Text("Total Duration: %.2f ms", data_->trace_reader->get_total_duration_ns() / 1e6);
            ImGui::Text("Operations: %zu", op_timings.size());
        }
        
        ImGui::Separator();
        
        // Tabs for different views
        if (ImGui::BeginTabBar("TimelineViews")) {
            // Visual Timeline tab
            if (ImGui::BeginTabItem("Visual Timeline")) {
                // Render the custom timeline widget
                const TraceReader* trace_reader_for_widget = nullptr;
                if (data_->live_mode && data_->live_trace_reader) {
                    trace_reader_for_widget = data_->live_trace_reader.get();
                } else {
                    trace_reader_for_widget = data_->trace_reader.get();
                }
                
                if (trace_reader_for_widget) {
                    timeline_widget_.render("##timeline", trace_reader_for_widget, timeline_config_);
                    
                    // Sync selection between timeline widget and event details
                    int selected = timeline_widget_.get_selected_event();
                    if (selected != data_->selected_event) {
                        data_->selected_event = selected;
                    }
                } else {
                    ImGui::Text("No trace data available for timeline visualization");
                }
                
                ImGui::EndTabItem();
            }
            
            // Events tab
            if (ImGui::BeginTabItem("Events")) {
                if (ImGui::BeginChild("EventList")) {
                    ImGuiListClipper clipper;
                    clipper.Begin(events.size());
                    
                    while (clipper.Step()) {
                        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                            const auto& event = events[i];
                            
                            bool is_selected = (data_->selected_event == i);
                            
                            // Format event info
                            std::string event_name;
                            switch(event.type) {
                                case EventType::GRAPH_COMPUTE_BEGIN:
                                    event_name = "GRAPH_BEGIN";
                                    break;
                                case EventType::GRAPH_COMPUTE_END:
                                    event_name = "GRAPH_END";
                                    break;
                                case EventType::OP_COMPUTE_BEGIN:
                                    event_name = "OP_BEGIN";
                                    break;
                                case EventType::OP_COMPUTE_END:
                                    event_name = "OP_END";
                                    break;
                                default:
                                    event_name = "UNKNOWN";
                            }
                            
                            std::string label = std::to_string(i) + ": " + event_name;
                            if (event.label) {
                                label += " (" + std::string(event.label) + ")";
                            }
                            
                            if (ImGui::Selectable(label.c_str(), is_selected)) {
                                data_->selected_event = i;
                                timeline_widget_.set_selected_event(i);
                            }
                            
                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::Text("Event: %s", event_name.c_str());
                                ImGui::Text("Timestamp: %.3f ms", event.timestamp_ns / 1e6);
                                ImGui::Text("Thread: %d", event.thread_id);
                                if (event.label) {
                                    ImGui::Text("Label: %s", event.label);
                                }
                                ImGui::EndTooltip();
                            }
                        }
                    }
                    clipper.End();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            
            // Operation timings tab
            if (ImGui::BeginTabItem("Op Timings")) {
                if (ImGui::BeginChild("OpTimings")) {
                    if (data_->live_mode) {
                        ImGui::Text("Live mode: Operation timings calculated on-the-fly");
                        ImGui::Text("Events collected: %zu", events.size());
                        // TODO: Calculate live operation timings from events
                    } else if (data_->trace_reader) {
                        auto op_timings = data_->trace_reader->get_op_timings();
                        uint64_t total_duration = data_->trace_reader->get_total_duration_ns();
                        
                        ImGui::Columns(3, "OpTimingsColumns");
                        ImGui::Text("Operation");
                        ImGui::NextColumn();
                        ImGui::Text("Duration");
                        ImGui::NextColumn();
                        ImGui::Text("%% of Total");
                        ImGui::NextColumn();
                        ImGui::Separator();
                        
                        for (size_t i = 0; i < op_timings.size(); i++) {
                            const auto& timing = op_timings[i];
                            
                            ImGui::Text("%s", timing.name.c_str());
                            ImGui::NextColumn();
                            ImGui::Text("%.3f ms", timing.duration_ns / 1e6);
                            ImGui::NextColumn();
                            if (total_duration > 0) {
                                ImGui::Text("%.1f%%", (timing.duration_ns * 100.0) / total_duration);
                            } else {
                                ImGui::Text("N/A");
                            }
                            ImGui::NextColumn();
                        }
                        ImGui::Columns(1);
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void ImGuiApp::render_graph_view() {
    if (ImGui::Begin("Graph View")) {
        if (!data_->trace_reader && !data_->live_mode) {
            ImGui::Text("No trace loaded and live mode not active");
            ImGui::End();
            return;
        }
        
        if (data_->live_mode) {
            ImGui::Text("🔴 LIVE MODE - Graph View");
            ImGui::Text("Live events: %zu", data_->live_events.size());
            
            // Count graph events in live mode
            size_t graph_begin_count = 0;
            size_t graph_end_count = 0;
            for (const auto& event : data_->live_events) {
                if (event.type == EventType::GRAPH_COMPUTE_BEGIN) graph_begin_count++;
                if (event.type == EventType::GRAPH_COMPUTE_END) graph_end_count++;
            }
            ImGui::Text("Graph Begin Events: %zu", graph_begin_count);
            ImGui::Text("Graph End Events: %zu", graph_end_count);
            
            ImGui::Separator();
            
            // Use live trace reader for graph visualization
            if (data_->live_trace_reader) {
                graph_widget_.render("##compute_graph", data_->live_trace_reader.get(), graph_config_);
                
                // Sync selection between graph widget and other views
                // TODO: Map node selection to event selection
                // int selected = graph_widget_.get_selected_node(); // Unused for now
            } else {
                ImGui::Text("Loading graph data...");
            }
            
        } else if (data_->trace_reader) {
            auto graph_events = data_->trace_reader->get_graph_events();
            ImGui::Text("Graph Events: %zu", graph_events.size());
            
            ImGui::Separator();
            
            // Render the graph widget
            graph_widget_.render("##compute_graph", data_->trace_reader.get(), graph_config_);
            
            // Sync selection between graph widget and other views
            int selected_node = graph_widget_.get_selected_node();
            if (selected_node >= 0) {
                // In a complete implementation, we would map node selection back to events
                // For now, just show that a node is selected
                ImGui::Text("Selected Node: %d", selected_node);
            }
        }
    }
    ImGui::End();
}

void ImGuiApp::render_tensor_inspector() {
    if (ImGui::Begin("Tensor Inspector")) {
        if (!data_->trace_reader && !data_->live_mode) {
            ImGui::Text("No trace loaded and live mode not active");
            ImGui::End();
            return;
        }
        
        if (data_->live_mode) {
            ImGui::Text("🔴 LIVE MODE - Tensor Inspector");
            ImGui::Text("Live events: %zu", data_->live_events.size());
            
            if (data_->selected_event >= 0 && 
                data_->selected_event < static_cast<int>(data_->live_events.size())) {
                const auto& event = data_->live_events[data_->selected_event];
                
                ImGui::Separator();
                ImGui::Text("Selected Live Event Details:");
                ImGui::Text("Type: %d", static_cast<int>(event.type));
                ImGui::Text("Timestamp: %llu ns", event.timestamp_ns);
                ImGui::Text("Thread ID: %d", event.thread_id);
                
                if (event.label) {
                    ImGui::Text("Label: %s", event.label);
                }
            } else {
                ImGui::Text("Select an event from the timeline to inspect");
            }
            
        } else if (data_->trace_reader) {
            if (data_->selected_event >= 0 && 
                data_->selected_event < static_cast<int>(data_->trace_reader->events().size())) {
                const auto& event = data_->trace_reader->events()[data_->selected_event];
                
                ImGui::Text("Selected Event Details:");
                ImGui::Text("Type: %d", static_cast<int>(event.type));
                ImGui::Text("Timestamp: %llu ns", event.timestamp_ns);
                ImGui::Text("Thread ID: %d", event.thread_id);
                
                if (event.label) {
                    ImGui::Text("Label: %s", event.label);
                }
                
                // TODO: Add tensor-specific inspection
            } else {
                ImGui::Text("Select an event from the timeline to inspect");
            }
        }
    }
    ImGui::End();
}

void ImGuiApp::render_memory_view() {
    if (ImGui::Begin("Memory View")) {
        if (!data_->trace_reader && !data_->live_mode) {
            ImGui::Text("No trace loaded and live mode not active");
            ImGui::End();
            return;
        }
        
        if (data_->live_mode) {
            render_live_memory_view();
        } else {
            render_static_memory_view();
        }
    }
    ImGui::End();
}

void ImGuiApp::render_static_memory_view() {
    if (!data_->trace_reader) return;
    
    auto memory_stats = data_->trace_reader->get_memory_stats();
    auto memory_events = data_->trace_reader->get_memory_events();
    
    // Memory Statistics Section
    if (ImGui::CollapsingHeader("Memory Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2, "MemoryStats");
        
        ImGui::Text("Total Allocations:");
        ImGui::NextColumn();
        ImGui::Text("%llu", static_cast<unsigned long long>(memory_stats.total_allocations));
        ImGui::NextColumn();
        
        ImGui::Text("Total Frees:");
        ImGui::NextColumn();
        ImGui::Text("%llu", static_cast<unsigned long long>(memory_stats.total_frees));
        ImGui::NextColumn();
        
        ImGui::Text("Bytes Allocated:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", memory_stats.bytes_allocated / 1024.0);
        ImGui::NextColumn();
        
        ImGui::Text("Bytes Freed:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", memory_stats.bytes_freed / 1024.0);
        ImGui::NextColumn();
        
        ImGui::Text("Peak Usage:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", memory_stats.peak_usage / 1024.0);
        ImGui::NextColumn();
        
        ImGui::Text("Current Usage:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", memory_stats.current_usage / 1024.0);
        ImGui::NextColumn();
        
        if (memory_stats.leaked_bytes > 0) {
            ImGui::Text("🔴 Leaked Memory:");
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%.2f KB", memory_stats.leaked_bytes / 1024.0);
        } else {
            ImGui::Text("✅ Memory Leaks:");
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "None detected");
        }
        ImGui::NextColumn();
        
        ImGui::Columns(1);
    }
    
    // Memory Timeline Section
    if (ImGui::CollapsingHeader("Memory Timeline", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (memory_events.empty()) {
            ImGui::Text("No memory events found in trace");
        } else {
            render_memory_timeline(memory_events);
        }
    }
    
    // Memory Events List Section
    if (ImGui::CollapsingHeader("Memory Events")) {
        if (memory_events.empty()) {
            ImGui::Text("No memory events found in trace");
        } else {
            render_memory_events_list(memory_events);
        }
    }
}

void ImGuiApp::render_live_memory_view() {
    ImGui::Text("🔴 LIVE MODE - Memory View");
    ImGui::Text("Live events: %zu", data_->live_events.size());
    
    // DEBUG: Show event type breakdown
    size_t graph_events = 0, op_events = 0, memory_events = 0, other_events = 0;
    for (const auto& event : data_->live_events) {
        switch (event.type) {
            case EventType::GRAPH_COMPUTE_BEGIN:
            case EventType::GRAPH_COMPUTE_END:
                graph_events++;
                break;
            case EventType::OP_COMPUTE_BEGIN:
            case EventType::OP_COMPUTE_END:
                op_events++;
                break;
            case EventType::TENSOR_ALLOC:
            case EventType::TENSOR_FREE:
                memory_events++;
                break;
            default:
                other_events++;
                break;
        }
    }
    
    ImGui::Text("🔍 DEBUG - Event breakdown:");
    ImGui::BulletText("Graph events: %zu", graph_events);
    ImGui::BulletText("Operation events: %zu", op_events);
    ImGui::BulletText("Memory events: %zu", memory_events);
    ImGui::BulletText("Other events: %zu", other_events);
    
    // Update cached memory stats incrementally
    update_live_memory_stats();
    
    // Count memory events for display
    size_t memory_event_count = memory_events;
    
    if (memory_event_count == 0) {
        ImGui::Text("🔍 No memory events in live trace yet...");
        ImGui::Separator();
        ImGui::Text("💡 To see memory events:");
        ImGui::BulletText("Run a GGML application with:");
        ImGui::Indent();
        ImGui::Text("env DYLD_INSERT_LIBRARIES=./libggml_viz_hook.dylib your_app");
        ImGui::Text("env GGML_VIZ_OUTPUT=trace.ggmlviz your_app");
        ImGui::Unindent();
        ImGui::BulletText("Or monitor an external trace file:");
        ImGui::Indent();
        ImGui::Text("./ggml-viz --live --no-hook trace_file.ggmlviz");
        ImGui::Unindent();
        ImGui::Separator();
        ImGui::Text("✅ Memory tracking is enabled and ready!");
        return;
    }
    
    // Live Memory Statistics
    if (ImGui::CollapsingHeader("Live Memory Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2, "LiveMemoryStats");
        
        ImGui::Text("Allocations:");
        ImGui::NextColumn();
        ImGui::Text("%zu", data_->live_total_allocs);
        ImGui::NextColumn();
        
        ImGui::Text("Frees:");
        ImGui::NextColumn();
        ImGui::Text("%zu", data_->live_total_frees);
        ImGui::NextColumn();
        
        ImGui::Text("Peak Usage:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", data_->live_peak_usage / 1024.0);
        ImGui::NextColumn();
        
        ImGui::Text("Current Usage:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", data_->live_current_usage / 1024.0);
        ImGui::NextColumn();
        
        size_t leaked_bytes = 0;
        for (const auto& pair : data_->live_allocations) {
            leaked_bytes += pair.second;
        }
        
        if (leaked_bytes > 0) {
            ImGui::Text("🔴 Active Allocs:");
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "%.2f KB", leaked_bytes / 1024.0);
        } else {
            ImGui::Text("✅ Active Allocs:");
            ImGui::NextColumn();
            ImGui::Text("0 bytes");
        }
        
        ImGui::Columns(1);
    }
    
    // Recent Memory Events
    if (ImGui::CollapsingHeader("Recent Memory Events")) {
        render_live_memory_events_list();
    }
}

void ImGuiApp::render_memory_timeline(const std::vector<const Event*>& memory_events) {
    if (memory_events.empty()) return;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    if (canvas_sz.y < 200.0f) canvas_sz.y = 200.0f;
    
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
    
    // Timeline bounds
    uint64_t min_time = memory_events.front()->timestamp_ns;
    uint64_t max_time = memory_events.back()->timestamp_ns;
    uint64_t time_range = max_time - min_time;
    
    if (time_range == 0) time_range = 1; // Avoid division by zero
    
    // Calculate memory usage over time
    std::vector<std::pair<float, float>> usage_points; // (x_pos, usage_kb)
    size_t current_usage = 0;
    size_t max_usage = 0;
    std::unordered_map<const void*, size_t> allocations;
    
    for (const auto* event : memory_events) {
        float x_pos = (float)(event->timestamp_ns - min_time) / time_range;
        
        if (event->type == EventType::TENSOR_ALLOC) {
            allocations[event->data.memory.ptr] = event->data.memory.size;
            current_usage += event->data.memory.size;
        } else if (event->type == EventType::TENSOR_FREE) {
            auto it = allocations.find(event->data.memory.ptr);
            if (it != allocations.end()) {
                current_usage -= it->second;
                allocations.erase(it);
            }
        }
        
        if (current_usage > max_usage) max_usage = current_usage;
        usage_points.push_back({x_pos, (float)current_usage / 1024.0f});
    }
    
    // Draw background
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));
    
    // Draw grid lines
    const int num_h_lines = 5;
    const int num_v_lines = 10;
    
    for (int i = 0; i <= num_h_lines; i++) {
        float y = canvas_p0.y + ((float)i / num_h_lines) * canvas_sz.y;
        draw_list->AddLine(ImVec2(canvas_p0.x, y), ImVec2(canvas_p1.x, y), IM_COL32(100, 100, 100, 100));
    }
    
    for (int i = 0; i <= num_v_lines; i++) {
        float x = canvas_p0.x + ((float)i / num_v_lines) * canvas_sz.x;
        draw_list->AddLine(ImVec2(x, canvas_p0.y), ImVec2(x, canvas_p1.y), IM_COL32(100, 100, 100, 100));
    }
    
    // Draw memory usage line
    if (usage_points.size() > 1 && max_usage > 0) {
        for (size_t i = 1; i < usage_points.size(); i++) {
            ImVec2 p1(canvas_p0.x + usage_points[i-1].first * canvas_sz.x, 
                     canvas_p1.y - (usage_points[i-1].second / (max_usage / 1024.0f)) * canvas_sz.y);
            ImVec2 p2(canvas_p0.x + usage_points[i].first * canvas_sz.x, 
                     canvas_p1.y - (usage_points[i].second / (max_usage / 1024.0f)) * canvas_sz.y);
            draw_list->AddLine(p1, p2, IM_COL32(100, 200, 255, 255), 2.0f);
        }
    }
    
    // Draw allocation/free markers
    for (const auto* event : memory_events) {
        float x_pos = (float)(event->timestamp_ns - min_time) / time_range;
        float x = canvas_p0.x + x_pos * canvas_sz.x;
        
        if (event->type == EventType::TENSOR_ALLOC) {
            draw_list->AddLine(ImVec2(x, canvas_p0.y), ImVec2(x, canvas_p1.y), IM_COL32(100, 255, 100, 150), 1.0f);
        } else if (event->type == EventType::TENSOR_FREE) {
            draw_list->AddLine(ImVec2(x, canvas_p0.y), ImVec2(x, canvas_p1.y), IM_COL32(255, 100, 100, 150), 1.0f);
        }
    }
    
    ImGui::Dummy(canvas_sz);
    
    // Timeline legend
    ImGui::Text("Memory Timeline");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "━━ Usage");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "│ Alloc");
    ImGui::SameLine();    
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "│ Free");
    ImGui::Text("Peak: %.2f KB", max_usage / 1024.0f);
}

void ImGuiApp::render_memory_events_list(const std::vector<const Event*>& memory_events) {
    if (ImGui::BeginTable("MemoryEvents", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Time (ns)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Tensor", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        // Show most recent events first (last 100)
        size_t start_idx = memory_events.size() > 100 ? memory_events.size() - 100 : 0;
        for (size_t i = memory_events.size(); i > start_idx; --i) {
            const Event* event = memory_events[i - 1];
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%llu", event->timestamp_ns);
            
            ImGui::TableSetColumnIndex(1);
            if (event->type == EventType::TENSOR_ALLOC) {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "ALLOC");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "FREE");
            }
            
            ImGui::TableSetColumnIndex(2);
            if (event->type == EventType::TENSOR_ALLOC) {
                ImGui::Text("%zu B", event->data.memory.size);
            } else {
                ImGui::Text("-");
            }
            
            ImGui::TableSetColumnIndex(3);
            if (event->label && strlen(event->label) > 0) {
                ImGui::Text("%s", event->label);
            } else {
                ImGui::Text("0x%p", event->data.memory.ptr);
            }
        }
        
        ImGui::EndTable();
    }
}

void ImGuiApp::update_live_memory_stats() {
    // Process only new events since last update
    size_t events_to_process = data_->live_events.size();
    if (events_to_process <= data_->live_memory_last_processed_event) {
        return; // No new events
    }
    
    // Process new events incrementally
    for (size_t i = data_->live_memory_last_processed_event; i < events_to_process; ++i) {
        const auto& event = data_->live_events[i];
        
        if (event.type == EventType::TENSOR_ALLOC) {
            data_->live_total_allocs++;
            data_->live_bytes_allocated += event.data.memory.size;
            data_->live_allocations[event.data.memory.ptr] = event.data.memory.size;
            data_->live_current_usage += event.data.memory.size;
            
            if (data_->live_current_usage > data_->live_peak_usage) {
                data_->live_peak_usage = data_->live_current_usage;
            }
        } else if (event.type == EventType::TENSOR_FREE) {
            data_->live_total_frees++;
            auto it = data_->live_allocations.find(event.data.memory.ptr);
            if (it != data_->live_allocations.end()) {
                data_->live_current_usage -= it->second;
                data_->live_allocations.erase(it);
            }
        }
    }
    
    data_->live_memory_last_processed_event = events_to_process;
}

void ImGuiApp::render_live_memory_events_list() {
    // Show only recent memory events without copying into temporary vector
    const size_t max_display_events = 100;
    size_t memory_events_shown = 0;
    
    if (ImGui::BeginTable("LiveMemoryEvents", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Time (ns)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Pointer", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        // Iterate backwards through events to show most recent first
        for (int i = static_cast<int>(data_->live_events.size()) - 1; i >= 0 && memory_events_shown < max_display_events; --i) {
            const auto& event = data_->live_events[i];
            
            if (event.type == EventType::TENSOR_ALLOC || event.type == EventType::TENSOR_FREE) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%llu", static_cast<unsigned long long>(event.timestamp_ns));
                
                ImGui::TableSetColumnIndex(1);
                if (event.type == EventType::TENSOR_ALLOC) {
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "ALLOC");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "FREE");
                }
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%p", event.data.memory.ptr);
                
                ImGui::TableSetColumnIndex(3);
                if (event.type == EventType::TENSOR_ALLOC) {
                    ImGui::Text("%.2f KB", event.data.memory.size / 1024.0);
                } else {
                    ImGui::Text("-");
                }
                
                memory_events_shown++;
            }
        }
        
        ImGui::EndTable();
    }
    
    if (memory_events_shown == max_display_events) {
        ImGui::Text("... (showing most recent %zu memory events)", max_display_events);
    }
}

} // namespace ggml_viz