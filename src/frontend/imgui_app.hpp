// src/frontend/imgui_app.hpp
#pragma once

#include <string>
#include <memory>
#include <vector>
#include "imgui_widgets.hpp"
#include "../instrumentation/ggml_hook.hpp" // For Event type

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
    
    // Enable live mode - reads from running GGML applications
    void enable_live_mode(bool no_hook = false, const std::string& trace_file = "");
    void disable_live_mode();
    bool is_live_mode() const;

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
    void render_static_memory_view();
    void render_live_memory_view();
    void render_memory_timeline(const std::vector<const Event*>& memory_events);
    void render_memory_events_list(const std::vector<const Event*>& memory_events);
    void render_live_memory_events_list();
    void update_live_memory_stats();
    void render_file_browser();
    
    // Live mode support
    void update_live_data();
    
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