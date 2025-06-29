// src/frontend/imgui_app.hpp
#pragma once

#include <string>
#include <memory>
#include "imgui_widgets.hpp"

namespace ggml_viz {

class TraceReader;

class ImGuiApp {
public:
    ImGuiApp();
    ~ImGuiApp();

    // Main application lifecycle
    int run();
    
    // Load a trace file for visualization
    bool load_trace_file(const std::string& filename);

private:
    struct AppData;
    std::unique_ptr<AppData> data_;
    
    // GUI lifecycle
    bool initialize();
    void shutdown();
    void render_frame();
    
    // GUI panels
    void render_main_menu_bar();
    void render_timeline_view();
    void render_graph_view();
    void render_tensor_inspector();
    void render_memory_view();
    void render_file_browser();
    
    // GUI state
    bool show_demo_window_ = false;
    bool show_timeline_ = true;
    bool show_graph_ = true;
    bool show_tensor_inspector_ = true;
    bool show_memory_view_ = true;
    bool show_file_browser_ = false;
    
    // Timeline widget
    TimelineWidget timeline_widget_;
    TimelineWidget::TimelineConfig timeline_config_;
    
    // Graph widget
    GraphWidget graph_widget_;
    GraphWidget::GraphConfig graph_config_;
};

} // namespace ggml_viz