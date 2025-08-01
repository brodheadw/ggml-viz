// src/frontend/imgui_widgets.cpp
#include "imgui_widgets.hpp"
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <queue>
#include <cmath>

namespace ggml_viz {

TimelineWidget::TimelineWidget() {
}

bool TimelineWidget::render(const char* label, const TraceReader* trace_reader, TimelineConfig& config) {
    if (!trace_reader || trace_reader->event_count() == 0) {
        ImGui::Text("No trace data available");
        return false;
    }
    
    bool changed = false;
    float total_duration_ms = trace_reader->get_total_duration_ns() / 1e6f;
    
    // Auto-adjust zoom if this is the first time or data changed
    if (last_total_duration_ != total_duration_ms) {
        config.zoom = 1.0f;
        config.scroll_x = 0.0f;
        last_total_duration_ = total_duration_ms;
    }
    
    // Process trace events into timeline events
    auto timeline_events = process_events(trace_reader);
    
    ImGui::PushID(label);
    
    // Render timeline controls
    render_timeline_controls(config, total_duration_ms);
    
    // Render the main timeline canvas
    render_timeline_canvas(timeline_events, config, total_duration_ms);
    
    ImGui::PopID();
    
    return changed;
}

std::vector<TimelineWidget::TimelineEvent> TimelineWidget::process_events(const TraceReader* trace_reader) {
    std::vector<TimelineEvent> timeline_events;
    const auto& events = trace_reader->events();
    
    if (events.empty()) return timeline_events;
    
    // Find thread IDs and create mapping
    thread_ids_.clear();
    std::unordered_map<int, int> thread_to_lane;
    
    for (const auto& event : events) {
        auto [it, inserted] = thread_to_lane.try_emplace(event.thread_id, thread_ids_.size());
        if (inserted) {
            thread_ids_.push_back(event.thread_id);
        }
    }
    
    // Process operation timings to create timeline events
    auto op_timings = trace_reader->get_op_timings();
    uint64_t first_timestamp = events.empty() ? 0 : events[0].timestamp_ns;
    
    for (const auto& timing : op_timings) {
        TimelineEvent timeline_event;
        timeline_event.event_index = timing.begin - &events[0];  // Calculate index
        timeline_event.start_time_ms = (timing.begin->timestamp_ns - first_timestamp) / 1e6f;
        timeline_event.duration_ms = timing.duration_ns / 1e6f;
        timeline_event.thread_id = timing.begin->thread_id;
        timeline_event.label = timing.name;
        timeline_event.is_graph_event = (timing.begin->type == EventType::GRAPH_COMPUTE_BEGIN);
        timeline_event.color = get_event_color(*timing.begin, timeline_event.event_index == selected_event_);
        
        timeline_events.push_back(timeline_event);
    }
    
    // Sort by start time
    std::sort(timeline_events.begin(), timeline_events.end(),
              [](const TimelineEvent& a, const TimelineEvent& b) {
                  return a.start_time_ms < b.start_time_ms;
              });
    
    return timeline_events;
}

void TimelineWidget::render_timeline_canvas(const std::vector<TimelineEvent>& timeline_events, 
                                           TimelineConfig& config, 
                                           float total_duration_ms) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size.y = config.height;
    
    // Ensure minimum canvas width
    canvas_size.x = std::max(canvas_size.x, 400.0f);
    
    // Calculate timeline dimensions
    int num_lanes = config.show_threads ? thread_ids_.size() : 1;
    float total_lane_height = num_lanes * (config.lane_height + config.padding);
    // float usable_height = std::min(canvas_size.y - 40.0f, total_lane_height); // Leave space for time ruler (unused for now)
    
    // Draw canvas background
    draw_list->AddRectFilled(canvas_pos, 
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                           IM_COL32(30, 30, 30, 255));
    
    // Draw time ruler at the top
    float ruler_height = 20.0f;
    draw_list->AddRectFilled(canvas_pos, 
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + ruler_height),
                           IM_COL32(40, 40, 40, 255));
    
    // Draw time markers
    float timeline_width = canvas_size.x;
    float visible_duration = total_duration_ms / config.zoom;
    float start_time = config.scroll_x * total_duration_ms;
    float end_time = start_time + visible_duration;
    
    // Calculate time step for markers
    float time_step = visible_duration / 10.0f; // Aim for ~10 markers
    float marker_step = 1.0f;
    if (time_step < 1.0f) marker_step = 0.1f;
    else if (time_step < 10.0f) marker_step = 1.0f;
    else if (time_step < 100.0f) marker_step = 10.0f;
    else marker_step = 100.0f;
    
    for (float t = 0; t <= total_duration_ms; t += marker_step) {
        if (t < start_time || t > end_time) continue;
        
        float x = time_to_pixel(t, total_duration_ms, timeline_width, config);
        draw_list->AddLine(ImVec2(canvas_pos.x + x, canvas_pos.y),
                          ImVec2(canvas_pos.x + x, canvas_pos.y + ruler_height),
                          IM_COL32(100, 100, 100, 255));
        
        // Draw time label
        std::string time_str = TimelineUtils::format_timestamp(t);
        ImVec2 text_size = ImGui::CalcTextSize(time_str.c_str());
        draw_list->AddText(ImVec2(canvas_pos.x + x - text_size.x * 0.5f, canvas_pos.y + 2),
                          IM_COL32(200, 200, 200, 255), time_str.c_str());
    }
    
    // Draw thread lane separators
    if (config.show_threads && num_lanes > 1) {
        for (int i = 1; i < num_lanes; i++) {
            float y = canvas_pos.y + ruler_height + i * (config.lane_height + config.padding);
            draw_list->AddLine(ImVec2(canvas_pos.x, y),
                              ImVec2(canvas_pos.x + canvas_size.x, y),
                              IM_COL32(60, 60, 60, 255));
        }
    }
    
    // Draw timeline events
    for (const auto& event : timeline_events) {
        float start_x = time_to_pixel(event.start_time_ms, total_duration_ms, timeline_width, config);
        float end_x = time_to_pixel(event.start_time_ms + event.duration_ms, total_duration_ms, timeline_width, config);
        
        // Skip events outside visible area
        if (end_x < 0 || start_x > timeline_width) continue;
        
        // Clamp to visible area
        start_x = std::max(0.0f, start_x);
        end_x = std::min(timeline_width, end_x);
        
        // Calculate lane position
        int lane = config.show_threads ? get_thread_lane(event.thread_id) : 0;
        float y = canvas_pos.y + ruler_height + lane * (config.lane_height + config.padding) + config.padding;
        
        // Draw event rectangle
        ImVec2 rect_min(canvas_pos.x + start_x, y);
        ImVec2 rect_max(canvas_pos.x + end_x, y + config.lane_height);
        
        // Ensure minimum width for visibility
        if (rect_max.x - rect_min.x < 2.0f) {
            rect_max.x = rect_min.x + 2.0f;
        }
        
        draw_list->AddRectFilled(rect_min, rect_max, event.color);
        
        // Draw border for selected event
        if (event.event_index == selected_event_) {
            draw_list->AddRect(rect_min, rect_max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
        }
        
        // Draw label if there's space and labels are enabled
        if (config.show_labels && (rect_max.x - rect_min.x) > 50.0f) {
            ImVec2 text_size = ImGui::CalcTextSize(event.label.c_str());
            if (text_size.x < (rect_max.x - rect_min.x) - 4.0f) {
                draw_list->AddText(ImVec2(rect_min.x + 2, rect_min.y + 2),
                                  IM_COL32(255, 255, 255, 255), event.label.c_str());
            }
        }
    }
    
    // Handle mouse interaction
    ImGui::InvisibleButton("timeline_canvas", canvas_size);
    if (ImGui::IsItemHovered()) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float mouse_x = mouse_pos.x - canvas_pos.x;
        float mouse_y = mouse_pos.y - canvas_pos.y - ruler_height;
        
        // Find event under mouse
        for (const auto& event : timeline_events) {
            float start_x = time_to_pixel(event.start_time_ms, total_duration_ms, timeline_width, config);
            float end_x = time_to_pixel(event.start_time_ms + event.duration_ms, total_duration_ms, timeline_width, config);
            
            int lane = config.show_threads ? get_thread_lane(event.thread_id) : 0;
            float y = lane * (config.lane_height + config.padding) + config.padding;
            
            if (mouse_x >= start_x && mouse_x <= end_x && 
                mouse_y >= y && mouse_y <= y + config.lane_height) {
                
                // Show tooltip
                ImGui::BeginTooltip();
                ImGui::Text("Operation: %s", event.label.c_str());
                ImGui::Text("Duration: %s", TimelineUtils::format_duration(event.duration_ms).c_str());
                ImGui::Text("Start: %s", TimelineUtils::format_timestamp(event.start_time_ms).c_str());
                ImGui::Text("Thread: %d", event.thread_id);
                ImGui::EndTooltip();
                
                // Handle click
                if (ImGui::IsMouseClicked(0)) {
                    selected_event_ = event.event_index;
                }
                break;
            }
        }
        
        // Handle wheel zoom
        if (ImGui::GetIO().MouseWheel != 0.0f) {
            float wheel = ImGui::GetIO().MouseWheel;
            float mouse_time = pixel_to_time(mouse_x, total_duration_ms, timeline_width, config);
            
            config.zoom *= (1.0f + wheel * 0.1f);
            config.zoom = std::max(0.1f, std::min(config.zoom, 100.0f));
            
            // Adjust scroll to keep mouse position stable
            float new_mouse_time = pixel_to_time(mouse_x, total_duration_ms, timeline_width, config);
            config.scroll_x += (mouse_time - new_mouse_time) / total_duration_ms;
            config.scroll_x = std::max(0.0f, std::min(config.scroll_x, 1.0f - 1.0f/config.zoom));
        }
    }
}

void TimelineWidget::render_timeline_controls(TimelineConfig& config, float total_duration_ms) {
    // Zoom control
    ImGui::Text("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("##zoom", &config.zoom, 0.1f, 10.0f, "%.1fx")) {
        config.scroll_x = std::max(0.0f, std::min(config.scroll_x, 1.0f - 1.0f/config.zoom));
    }
    
    // Scroll control
    ImGui::SameLine();
    ImGui::Text("Scroll:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    float max_scroll = std::max(0.0f, 1.0f - 1.0f/config.zoom);
    ImGui::SliderFloat("##scroll", &config.scroll_x, 0.0f, max_scroll, "%.2f");
    
    // Display options
    ImGui::SameLine();
    ImGui::Checkbox("Labels", &config.show_labels);
    ImGui::SameLine();
    ImGui::Checkbox("Threads", &config.show_threads);
    
    // Stats
    ImGui::SameLine();
    ImGui::Text("| Duration: %s", TimelineUtils::format_duration(total_duration_ms).c_str());
}

ImU32 TimelineWidget::get_event_color(const Event& event, bool is_selected) {
    if (is_selected) {
        return IM_COL32(255, 200, 100, 255);  // Highlighted color
    }
    
    return TimelineUtils::get_operation_color(event);
}

int TimelineWidget::get_thread_lane(int thread_id) {
    auto it = std::find(thread_ids_.begin(), thread_ids_.end(), thread_id);
    return (it != thread_ids_.end()) ? (it - thread_ids_.begin()) : 0;
}

float TimelineWidget::time_to_pixel(float time_ms, float total_duration_ms, float canvas_width, const TimelineConfig& config) {
    if (total_duration_ms <= 0) return 0;
    
    float visible_duration = total_duration_ms / config.zoom;
    float start_time = config.scroll_x * total_duration_ms;
    return ((time_ms - start_time) / visible_duration) * canvas_width;
}

float TimelineWidget::pixel_to_time(float pixel_x, float total_duration_ms, float canvas_width, const TimelineConfig& config) {
    if (canvas_width <= 0 || total_duration_ms <= 0) return 0;
    
    float visible_duration = total_duration_ms / config.zoom;
    float start_time = config.scroll_x * total_duration_ms;
    return start_time + (pixel_x / canvas_width) * visible_duration;
}

// TimelineUtils implementation
namespace TimelineUtils {

ImU32 get_thread_color(int thread_id) {
    // Generate distinct colors for different threads
    static const ImU32 colors[] = {
        IM_COL32(100, 150, 255, 255),  // Blue
        IM_COL32(255, 100, 150, 255),  // Pink
        IM_COL32(150, 255, 100, 255),  // Green
        IM_COL32(255, 255, 100, 255),  // Yellow
        IM_COL32(255, 150, 100, 255),  // Orange
        IM_COL32(150, 100, 255, 255),  // Purple
        IM_COL32(100, 255, 255, 255),  // Cyan
        IM_COL32(255, 100, 100, 255),  // Red
    };
    
    return colors[thread_id % (sizeof(colors) / sizeof(colors[0]))];
}

ImU32 get_operation_color(const Event& event) {
    switch (event.type) {
        case EventType::GRAPH_COMPUTE_BEGIN:
        case EventType::GRAPH_COMPUTE_END:
            return IM_COL32(100, 200, 100, 255);  // Green for graph operations
        case EventType::OP_COMPUTE_BEGIN:
        case EventType::OP_COMPUTE_END:
            return get_thread_color(event.thread_id);  // Color by thread
        default:
            return IM_COL32(150, 150, 150, 255);  // Gray for unknown
    }
}

std::string format_duration(float duration_ms) {
    std::stringstream ss;
    if (duration_ms < 1.0f) {
        ss << std::fixed << std::setprecision(3) << (duration_ms * 1000.0f) << " μs";
    } else if (duration_ms < 1000.0f) {
        ss << std::fixed << std::setprecision(2) << duration_ms << " ms";
    } else {
        ss << std::fixed << std::setprecision(2) << (duration_ms / 1000.0f) << " s";
    }
    return ss.str();
}

std::string format_timestamp(float timestamp_ms) {
    std::stringstream ss;
    if (timestamp_ms < 1000.0f) {
        ss << std::fixed << std::setprecision(1) << timestamp_ms << "ms";
    } else {
        ss << std::fixed << std::setprecision(2) << (timestamp_ms / 1000.0f) << "s";
    }
    return ss.str();
}

} // namespace TimelineUtils

// GraphWidget implementation
GraphWidget::GraphWidget() {}

bool GraphWidget::render(const char* label, const TraceReader* trace_reader, GraphConfig& config) {
    if (!trace_reader || trace_reader->event_count() == 0) {
        ImGui::Text("No trace data available for graph visualization");
        return false;
    }
    
    ImGui::PushID(label);
    
    // Rebuild graph if needed
    if (nodes_dirty_) {
        cached_nodes_ = build_graph_from_trace(trace_reader);
        if (config.auto_layout) {
            auto_layout_nodes(cached_nodes_, config);
        }
        nodes_dirty_ = false;
    }
    
    // Render graph controls
    render_graph_controls(config);
    
    // Render the graph canvas
    render_graph_canvas(cached_nodes_, config);
    
    ImGui::PopID();
    
    return false; // No changes for now
}

std::vector<GraphWidget::GraphNode> GraphWidget::build_graph_from_trace(const TraceReader* trace_reader) {
    std::vector<GraphNode> nodes;
    const auto& events = trace_reader->events();
    auto op_timings = trace_reader->get_op_timings();
    
    // Create a map of tensor pointers to node IDs
    std::unordered_map<const void*, int> tensor_to_node;
    
    int node_id = 0;
    
    // Build nodes from operation timings
    for (const auto& timing : op_timings) {
        GraphNode node;
        node.node_id = node_id++;
        node.label = timing.name.empty() ? ("op_" + std::to_string(node.node_id)) : timing.name;
        node.duration_ms = timing.duration_ns / 1e6f;
        node.tensor_ptr = timing.begin->data.op.tensor_ptr;
        node.size = ImVec2(120.0f, 60.0f);
        node.is_selected = false;
        
        // Try to determine operation type from event data or label
        if (timing.begin && timing.begin->data.op.op_type != 0) {
            node.op_type = "op_" + std::to_string(timing.begin->data.op.op_type);
        } else {
            // Extract op type from label if possible
            std::string label_str = node.label;
            if (label_str.find("add") != std::string::npos) node.op_type = "ADD";
            else if (label_str.find("mul") != std::string::npos) node.op_type = "MUL";
            else if (label_str.find("conv") != std::string::npos) node.op_type = "CONV";
            else if (label_str.find("linear") != std::string::npos) node.op_type = "LINEAR";
            else if (label_str.find("softmax") != std::string::npos) node.op_type = "SOFTMAX";
            else if (label_str.find("relu") != std::string::npos) node.op_type = "RELU";
            else if (label_str.find("norm") != std::string::npos) node.op_type = "NORM";
            else node.op_type = "UNKNOWN";
        }
        
        node.color = get_op_color(node.op_type);
        
        // Map tensor pointer to node ID for later connection building
        // TODO: Use tensor_to_node for dependency analysis
        (void)tensor_to_node[node.tensor_ptr]; // Suppress unused warning
        
        nodes.push_back(node);
    }
    
    // For now, create a simple linear connection pattern
    // In a real implementation, this would analyze tensor dependencies
    for (size_t i = 1; i < nodes.size(); i++) {
        nodes[i].inputs.push_back(nodes[i-1].node_id);
        nodes[i-1].outputs.push_back(nodes[i].node_id);
    }
    
    return nodes;
}

void GraphWidget::auto_layout_nodes(std::vector<GraphNode>& nodes, const GraphConfig& config) {
    if (nodes.empty()) return;
    
    // Simple layered layout algorithm
    // Group nodes by their depth in the graph
    std::vector<std::vector<int>> layers;
    std::vector<int> node_depths(nodes.size(), -1);
    
    // Find root nodes (no inputs)
    std::vector<int> roots;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i].inputs.empty()) {
            roots.push_back(i);
            node_depths[i] = 0;
        }
    }
    
    // If no clear roots, start with first node
    if (roots.empty() && !nodes.empty()) {
        roots.push_back(0);
        node_depths[0] = 0;
    }
    
    // Breadth-first traversal to assign depths
    std::queue<int> queue;
    for (int root : roots) {
        queue.push(root);
    }
    
    int max_depth = 0;
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        int current_depth = node_depths[current];
        max_depth = std::max(max_depth, current_depth);
        
        for (int output_id : nodes[current].outputs) {
            if (output_id < nodes.size() && node_depths[output_id] < current_depth + 1) {
                node_depths[output_id] = current_depth + 1;
                queue.push(output_id);
            }
        }
    }
    
    // Group nodes by depth
    layers.resize(max_depth + 1);
    for (size_t i = 0; i < nodes.size(); i++) {
        int depth = node_depths[i];
        if (depth >= 0) {
            layers[depth].push_back(i);
        } else {
            // Handle disconnected nodes
            layers[0].push_back(i);
        }
    }
    
    // Position nodes in layers
    for (size_t layer = 0; layer < layers.size(); layer++) {
        const auto& layer_nodes = layers[layer];
        float y = layer * config.node_spacing_y;
        
        // Center nodes horizontally in each layer
        float total_width = (layer_nodes.size() - 1) * config.node_spacing_x;
        float start_x = -total_width * 0.5f;
        
        for (size_t i = 0; i < layer_nodes.size(); i++) {
            int node_idx = layer_nodes[i];
            nodes[node_idx].position = ImVec2(start_x + i * config.node_spacing_x, y);
            nodes[node_idx].size = ImVec2(config.node_width, config.node_height);
        }
    }
}

void GraphWidget::render_graph_canvas(std::vector<GraphNode>& nodes, GraphConfig& config) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    // Minimum canvas size
    canvas_size.x = std::max(canvas_size.x, 400.0f);
    canvas_size.y = std::max(canvas_size.y, 300.0f);
    
    // Draw canvas background
    draw_list->AddRectFilled(canvas_pos, 
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                           IM_COL32(25, 25, 25, 255));
    
    // Draw grid
    float grid_step = 50.0f * config.zoom;
    if (grid_step > 10.0f) {
        ImU32 grid_color = IM_COL32(50, 50, 50, 100);
        
        // Vertical lines
        for (float x = fmod(config.pan_offset.x, grid_step); x < canvas_size.x; x += grid_step) {
            draw_list->AddLine(ImVec2(canvas_pos.x + x, canvas_pos.y),
                              ImVec2(canvas_pos.x + x, canvas_pos.y + canvas_size.y),
                              grid_color);
        }
        
        // Horizontal lines
        for (float y = fmod(config.pan_offset.y, grid_step); y < canvas_size.y; y += grid_step) {
            draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + y),
                              ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + y),
                              grid_color);
        }
    }
    
    // Render connections first (so they appear behind nodes)
    render_connections(draw_list, nodes, config, canvas_pos);
    
    // Render nodes
    ImVec2 mouse_pos = ImGui::GetMousePos();
    int hovered_node = -1;
    
    for (auto& node : nodes) {
        ImVec2 screen_pos = graph_to_screen(node.position, config, canvas_pos);
        
        // Check if node is visible
        if (screen_pos.x + node.size.x * config.zoom < canvas_pos.x ||
            screen_pos.x > canvas_pos.x + canvas_size.x ||
            screen_pos.y + node.size.y * config.zoom < canvas_pos.y ||
            screen_pos.y > canvas_pos.y + canvas_size.y) {
            continue;
        }
        
        // Check if mouse is over this node
        bool is_hovered = (mouse_pos.x >= screen_pos.x && 
                          mouse_pos.x <= screen_pos.x + node.size.x * config.zoom &&
                          mouse_pos.y >= screen_pos.y && 
                          mouse_pos.y <= screen_pos.y + node.size.y * config.zoom);
        
        if (is_hovered) {
            hovered_node = node.node_id;
        }
        
        node.is_selected = (selected_node_ == node.node_id);
        render_node(draw_list, node, config, canvas_pos, is_hovered);
    }
    
    // Handle mouse interaction
    ImGui::InvisibleButton("graph_canvas", canvas_size);
    
    if (ImGui::IsItemHovered()) {
        // Handle node selection
        if (ImGui::IsMouseClicked(0) && hovered_node >= 0) {
            selected_node_ = hovered_node;
        }
        
        // Handle panning
        if (ImGui::IsMouseDragging(0) && hovered_node < 0) {
            ImVec2 delta = ImGui::GetMouseDragDelta(0);
            config.pan_offset.x += delta.x;
            config.pan_offset.y += delta.y;
            ImGui::ResetMouseDragDelta(0);
        }
        
        // Handle zoom
        if (ImGui::GetIO().MouseWheel != 0.0f) {
            float wheel = ImGui::GetIO().MouseWheel;
            float old_zoom = config.zoom;
            config.zoom *= (1.0f + wheel * 0.1f);
            config.zoom = std::max(0.1f, std::min(config.zoom, 5.0f));
            
            // Adjust pan to zoom towards mouse
            ImVec2 mouse_graph = screen_to_graph(mouse_pos, config, canvas_pos);
            config.pan_offset.x += (mouse_pos.x - canvas_pos.x) * (1.0f - config.zoom / old_zoom);
            config.pan_offset.y += (mouse_pos.y - canvas_pos.y) * (1.0f - config.zoom / old_zoom);
        }
        
        // Show tooltip for hovered node
        if (hovered_node >= 0 && hovered_node < nodes.size()) {
            const auto& node = nodes[hovered_node];
            ImGui::BeginTooltip();
            ImGui::Text("Node: %s", node.label.c_str());
            ImGui::Text("Type: %s", node.op_type.c_str());
            if (config.show_timing) {
                ImGui::Text("Duration: %.3f ms", node.duration_ms);
            }
            ImGui::Text("Inputs: %zu, Outputs: %zu", node.inputs.size(), node.outputs.size());
            ImGui::EndTooltip();
        }
    }
}

void GraphWidget::render_graph_controls(GraphConfig& config) {
    // Layout controls
    ImGui::Text("Layout:");
    ImGui::SameLine();
    if (ImGui::Button("Auto Layout")) {
        if (!cached_nodes_.empty()) {
            auto_layout_nodes(cached_nodes_, config);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset View")) {
        config.zoom = 1.0f;
        config.pan_offset = ImVec2(0.0f, 0.0f);
    }
    
    // Display options
    ImGui::SameLine();
    ImGui::Checkbox("Op Types", &config.show_op_types);
    ImGui::SameLine();
    ImGui::Checkbox("Timing", &config.show_timing);
    
    // Zoom control
    ImGui::SameLine();
    ImGui::Text("Zoom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##zoom", &config.zoom, 0.1f, 5.0f, "%.1fx");
}

void GraphWidget::render_node(ImDrawList* draw_list, const GraphNode& node, const GraphConfig& config, 
                             const ImVec2& canvas_pos, bool is_hovered) {
    ImVec2 screen_pos = graph_to_screen(node.position, config, canvas_pos);
    ImVec2 size = ImVec2(node.size.x * config.zoom, node.size.y * config.zoom);
    
    // Node background
    ImU32 bg_color = node.color;
    if (node.is_selected) {
        bg_color = IM_COL32(255, 200, 100, 255);  // Highlight selected
    } else if (is_hovered) {
        // Brighten color for hover
        ImU32 r = (bg_color >> 0) & 0xFF;
        ImU32 g = (bg_color >> 8) & 0xFF;
        ImU32 b = (bg_color >> 16) & 0xFF;
        r = std::min(255u, r + 40);
        g = std::min(255u, g + 40);
        b = std::min(255u, b + 40);
        bg_color = IM_COL32(r, g, b, 255);
    }
    
    // Draw node rectangle
    draw_list->AddRectFilled(screen_pos, 
                           ImVec2(screen_pos.x + size.x, screen_pos.y + size.y),
                           bg_color, 5.0f * config.zoom);
    
    // Draw node border
    ImU32 border_color = node.is_selected ? IM_COL32(255, 255, 0, 255) : IM_COL32(100, 100, 100, 255);
    draw_list->AddRect(screen_pos, 
                      ImVec2(screen_pos.x + size.x, screen_pos.y + size.y),
                      border_color, 5.0f * config.zoom, 0, 2.0f);
    
    // Draw node text
    if (config.zoom > 0.5f) {  // Only draw text if zoomed in enough
        ImVec2 text_pos = ImVec2(screen_pos.x + 5 * config.zoom, screen_pos.y + 5 * config.zoom);
        
        // Node label
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), node.label.c_str());
        
        // Operation type
        if (config.show_op_types && !node.op_type.empty()) {
            text_pos.y += 15 * config.zoom;
            draw_list->AddText(text_pos, IM_COL32(200, 200, 200, 255), node.op_type.c_str());
        }
        
        // Timing information
        if (config.show_timing && node.duration_ms > 0) {
            text_pos.y += 15 * config.zoom;
            std::string timing = TimelineUtils::format_duration(node.duration_ms);
            draw_list->AddText(text_pos, IM_COL32(255, 255, 100, 255), timing.c_str());
        }
    }
}

void GraphWidget::render_connections(ImDrawList* draw_list, const std::vector<GraphNode>& nodes, 
                                   const GraphConfig& config, const ImVec2& canvas_pos) {
    ImU32 connection_color = IM_COL32(150, 150, 150, 255);
    float thickness = 2.0f * config.zoom;
    
    for (const auto& node : nodes) {
        ImVec2 node_center = graph_to_screen(
            ImVec2(node.position.x + node.size.x * 0.5f, node.position.y + node.size.y * 0.5f),
            config, canvas_pos);
        
        // Draw connections to output nodes
        for (int output_id : node.outputs) {
            if (output_id < nodes.size()) {
                const auto& output_node = nodes[output_id];
                ImVec2 output_center = graph_to_screen(
                    ImVec2(output_node.position.x + output_node.size.x * 0.5f, 
                           output_node.position.y + output_node.size.y * 0.5f),
                    config, canvas_pos);
                
                // Draw curved connection
                ImVec2 cp1 = ImVec2(node_center.x + 50 * config.zoom, node_center.y);
                ImVec2 cp2 = ImVec2(output_center.x - 50 * config.zoom, output_center.y);
                
                draw_list->AddBezierCubic(node_center, cp1, cp2, output_center, 
                                        connection_color, thickness);
                
                // Draw arrow at the end
                ImVec2 arrow_dir = ImVec2(output_center.x - cp2.x, output_center.y - cp2.y);
                float arrow_len = sqrt(arrow_dir.x * arrow_dir.x + arrow_dir.y * arrow_dir.y);
                if (arrow_len > 0) {
                    arrow_dir.x /= arrow_len;
                    arrow_dir.y /= arrow_len;
                    
                    ImVec2 arrow_p1 = ImVec2(output_center.x - 10 * config.zoom * arrow_dir.x + 5 * config.zoom * arrow_dir.y,
                                           output_center.y - 10 * config.zoom * arrow_dir.y - 5 * config.zoom * arrow_dir.x);
                    ImVec2 arrow_p2 = ImVec2(output_center.x - 10 * config.zoom * arrow_dir.x - 5 * config.zoom * arrow_dir.y,
                                           output_center.y - 10 * config.zoom * arrow_dir.y + 5 * config.zoom * arrow_dir.x);
                    
                    draw_list->AddTriangleFilled(output_center, arrow_p1, arrow_p2, connection_color);
                }
            }
        }
    }
}

ImVec2 GraphWidget::graph_to_screen(const ImVec2& graph_pos, const GraphConfig& config, const ImVec2& canvas_pos) {
    return ImVec2(canvas_pos.x + (graph_pos.x + config.pan_offset.x) * config.zoom,
                  canvas_pos.y + (graph_pos.y + config.pan_offset.y) * config.zoom);
}

ImVec2 GraphWidget::screen_to_graph(const ImVec2& screen_pos, const GraphConfig& config, const ImVec2& canvas_pos) {
    return ImVec2((screen_pos.x - canvas_pos.x) / config.zoom - config.pan_offset.x,
                  (screen_pos.y - canvas_pos.y) / config.zoom - config.pan_offset.y);
}

// Currently unused function - commented out to fix lint warning
/*
bool GraphWidget::point_in_node(const ImVec2& point, const GraphNode& node) {
    return point.x >= node.position.x && point.x <= node.position.x + node.size.x &&
           point.y >= node.position.y && point.y <= node.position.y + node.size.y;
}
*/

ImU32 GraphWidget::get_op_color(const std::string& op_type) {
    // Color coding for different operation types
    if (op_type == "ADD" || op_type == "SUB") return IM_COL32(100, 200, 100, 255);      // Green - arithmetic
    if (op_type == "MUL" || op_type == "DIV") return IM_COL32(200, 100, 100, 255);      // Red - arithmetic
    if (op_type == "CONV") return IM_COL32(100, 100, 200, 255);                         // Blue - convolution
    if (op_type == "LINEAR") return IM_COL32(200, 100, 200, 255);                       // Purple - linear
    if (op_type == "SOFTMAX") return IM_COL32(200, 200, 100, 255);                      // Yellow - activation
    if (op_type == "RELU") return IM_COL32(100, 200, 200, 255);                         // Cyan - activation
    if (op_type == "NORM") return IM_COL32(150, 150, 200, 255);                         // Light blue - normalization
    
    return IM_COL32(120, 120, 120, 255);  // Gray - unknown
}

} // namespace ggml_viz