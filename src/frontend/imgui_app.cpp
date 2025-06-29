// src/frontend/imgui_app.cpp
#include "imgui_app.hpp"
#include "utils/trace_reader.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

namespace ggml_viz {

struct ImGuiApp::AppData {
    GLFWwindow* window = nullptr;
    std::unique_ptr<TraceReader> trace_reader;
    std::string current_filename;
    std::string error_message;
    
    // UI state
    bool trace_loaded = false;
    int selected_event = -1;
    
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
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(data_->window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    
    return true;
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
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Note: Docking disabled for compatibility with basic ImGui build
    
    // Render main menu bar
    render_main_menu_bar();
    
    // Render GUI panels
    if (show_file_browser_) {
        render_file_browser();
    }
    
    if (data_->trace_loaded && data_->trace_reader) {
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
        auto reader = std::make_unique<TraceReader>(filename);
        if (!reader->is_valid()) {
            data_->error_message = "Failed to load trace file: " + filename;
            return false;
        }
        
        data_->trace_reader = std::move(reader);
        data_->trace_loaded = true;
        data_->current_filename = filename;
        data_->selected_event = -1;
        
        return true;
    } catch (const std::exception& e) {
        data_->error_message = "Error loading trace: " + std::string(e.what());
        return false;
    }
}

void ImGuiApp::render_timeline_view() {
    if (ImGui::Begin("Timeline View")) {
        if (!data_->trace_reader) {
            ImGui::Text("No trace loaded");
            ImGui::End();
            return;
        }
        
        const auto& events = data_->trace_reader->events();
        auto op_timings = data_->trace_reader->get_op_timings();
        
        // Summary information
        ImGui::Text("Total Events: %zu", events.size());
        ImGui::Text("Total Duration: %.2f ms", data_->trace_reader->get_total_duration_ns() / 1e6);
        ImGui::Text("Operations: %zu", op_timings.size());
        
        ImGui::Separator();
        
        // Tabs for different views
        if (ImGui::BeginTabBar("TimelineViews")) {
            // Visual Timeline tab
            if (ImGui::BeginTabItem("Visual Timeline")) {
                // Render the custom timeline widget
                timeline_widget_.render("##timeline", data_->trace_reader.get(), timeline_config_);
                
                // Sync selection between timeline widget and event details
                int selected = timeline_widget_.get_selected_event();
                if (selected != data_->selected_event) {
                    data_->selected_event = selected;
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
                    ImGui::Columns(3, "OpTimingsColumns");
                    ImGui::Text("Operation");
                    ImGui::NextColumn();
                    ImGui::Text("Duration");
                    ImGui::NextColumn();
                    ImGui::Text("%% of Total");
                    ImGui::NextColumn();
                    ImGui::Separator();
                    
                    uint64_t total_duration = data_->trace_reader->get_total_duration_ns();
                    
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
        if (!data_->trace_reader) {
            ImGui::Text("No trace loaded");
            ImGui::End();
            return;
        }
        
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
    ImGui::End();
}

void ImGuiApp::render_tensor_inspector() {
    if (ImGui::Begin("Tensor Inspector")) {
        if (!data_->trace_reader) {
            ImGui::Text("No trace loaded");
            ImGui::End();
            return;
        }
        
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
    ImGui::End();
}

void ImGuiApp::render_memory_view() {
    if (ImGui::Begin("Memory View")) {
        if (!data_->trace_reader) {
            ImGui::Text("No trace loaded");
            ImGui::End();
            return;
        }
        
        // TODO: Implement memory visualization
        ImGui::Text("Memory visualization coming soon...");
    }
    ImGui::End();
}

} // namespace ggml_viz