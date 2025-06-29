// src/frontend/imgui_widgets.cpp
#include "imgui_widgets.hpp"
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <iomanip>

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
        if (thread_to_lane.find(event.thread_id) == thread_to_lane.end()) {
            thread_to_lane[event.thread_id] = thread_ids_.size();
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
    float usable_height = std::min(canvas_size.y - 40.0f, total_lane_height); // Leave space for time ruler
    
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
            
            float old_zoom = config.zoom;
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

} // namespace ggml_viz