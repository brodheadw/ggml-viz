// src/frontend/imgui_widgets.hpp
#pragma once

#include "imgui.h"
#include "utils/trace_reader.hpp"
#include <vector>

namespace ggml_viz {

// Timeline visualization widget for displaying GGML trace events
class TimelineWidget {
public:
    struct TimelineConfig {
        float height = 200.0f;              // Timeline height in pixels
        float zoom = 1.0f;                  // Zoom level (1.0 = fit all data)
        float scroll_x = 0.0f;              // Horizontal scroll position
        bool show_labels = true;            // Show operation labels
        bool show_threads = true;           // Show thread lanes
        float lane_height = 20.0f;          // Height of each thread lane
        float padding = 2.0f;               // Padding between lanes
    };

    TimelineWidget();
    
    // Render the timeline widget
    bool render(const char* label, const TraceReader* trace_reader, TimelineConfig& config);
    
    // Get currently selected event (returns -1 if none)
    int get_selected_event() const { return selected_event_; }
    
    // Set the selected event
    void set_selected_event(int event_idx) { selected_event_ = event_idx; }

private:
    struct TimelineEvent {
        int event_index;
        float start_time_ms;
        float duration_ms;
        int thread_id;
        std::string label;
        ImU32 color;
        bool is_graph_event;
    };
    
    // Convert trace events to timeline events
    std::vector<TimelineEvent> process_events(const TraceReader* trace_reader);
    
    // Render the timeline canvas
    void render_timeline_canvas(const std::vector<TimelineEvent>& timeline_events, 
                               TimelineConfig& config, 
                               float total_duration_ms);
    
    // Render timeline controls (zoom, scroll, etc.)
    void render_timeline_controls(TimelineConfig& config, float total_duration_ms);
    
    // Get color for different event types
    ImU32 get_event_color(const Event& event, bool is_selected);
    
    // Get thread lane for a thread ID
    int get_thread_lane(int thread_id);
    
    // Convert time to pixel position
    float time_to_pixel(float time_ms, float total_duration_ms, float canvas_width, const TimelineConfig& config);
    
    // Convert pixel position to time
    float pixel_to_time(float pixel_x, float total_duration_ms, float canvas_width, const TimelineConfig& config);

    int selected_event_ = -1;
    std::vector<int> thread_ids_;          // Map thread IDs to lane indices
    float last_total_duration_ = 0.0f;     // Cache for auto-zoom calculations
};

// Utility functions for timeline rendering
namespace TimelineUtils {
    // Generate distinct colors for different threads
    ImU32 get_thread_color(int thread_id);
    
    // Generate colors for different operation types
    ImU32 get_operation_color(const Event& event);
    
    // Format time duration for display
    std::string format_duration(float duration_ms);
    
    // Format timestamp for display  
    std::string format_timestamp(float timestamp_ms);
}

} // namespace ggml_viz