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

// Compute graph visualization widget
class GraphWidget {
public:
    struct GraphConfig {
        float node_width = 120.0f;
        float node_height = 60.0f;
        float node_spacing_x = 160.0f;
        float node_spacing_y = 100.0f;
        bool show_op_types = true;
        bool show_timing = true;
        bool auto_layout = true;
        float zoom = 1.0f;
        ImVec2 pan_offset{0.0f, 0.0f};
    };

    struct GraphNode {
        int node_id;
        ImVec2 position;
        ImVec2 size;
        std::string label;
        std::string op_type;
        float duration_ms;
        std::vector<int> inputs;  // Node IDs of input nodes
        std::vector<int> outputs; // Node IDs of output nodes
        ImU32 color;
        const void* tensor_ptr;   // For linking to trace events
        bool is_selected;
    };

    GraphWidget();
    
    // Render the graph widget
    bool render(const char* label, const TraceReader* trace_reader, GraphConfig& config);
    
    // Get currently selected node
    int get_selected_node() const { return selected_node_; }
    
    // Set the selected node
    void set_selected_node(int node_id) { selected_node_ = node_id; }

private:
    // Build graph structure from trace data
    std::vector<GraphNode> build_graph_from_trace(const TraceReader* trace_reader);
    
    // Auto-layout the graph nodes
    void auto_layout_nodes(std::vector<GraphNode>& nodes, const GraphConfig& config);
    
    // Render the graph canvas
    void render_graph_canvas(std::vector<GraphNode>& nodes, GraphConfig& config);
    
    // Render graph controls
    void render_graph_controls(GraphConfig& config);
    
    // Render a single node
    void render_node(ImDrawList* draw_list, const GraphNode& node, const GraphConfig& config, 
                     const ImVec2& canvas_pos, bool is_hovered);
    
    // Render connections between nodes
    void render_connections(ImDrawList* draw_list, const std::vector<GraphNode>& nodes, 
                           const GraphConfig& config, const ImVec2& canvas_pos);
    
    // Convert graph coordinates to screen coordinates
    ImVec2 graph_to_screen(const ImVec2& graph_pos, const GraphConfig& config, const ImVec2& canvas_pos);
    
    // Convert screen coordinates to graph coordinates
    ImVec2 screen_to_graph(const ImVec2& screen_pos, const GraphConfig& config, const ImVec2& canvas_pos);
    
    // Check if point is inside node (currently unused)
    // bool point_in_node(const ImVec2& point, const GraphNode& node);
    
    // Get color for operation type
    ImU32 get_op_color(const std::string& op_type);
    
    int selected_node_ = -1;
    std::vector<GraphNode> cached_nodes_;
    bool nodes_dirty_ = true;
};

} // namespace ggml_viz