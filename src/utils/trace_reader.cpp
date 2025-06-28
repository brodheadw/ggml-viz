// src/utils/trace_reader.cpp
#include "trace_reader.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_map>

namespace ggml_viz {

TraceReader::TraceReader(const std::string& filename) : filename_(filename) {
    file_ = fopen(filename_.c_str(), "rb");
    if (!file_) {
        std::cerr << "Failed to open trace file: " << filename_ << "\n";
        return;
    }

    // Read header
    if (fread(&header_, sizeof(header_), 1, file_) != 1) {
        std::cerr << "Failed to read trace header\n";
        fclose(file_);
        file_ = nullptr;
        return;
    }

    // Verify magic
    if (strncmp(header_.magic, "GGMLVIZ1", 8) != 0) {
        std::cerr << "Invalid trace file magic\n";
        fclose(file_);
        file_ = nullptr;
        return;
    }

    valid_ = load_events();
}

TraceReader::~TraceReader() {
    if (file_) {
        fclose(file_);
    }
}

bool TraceReader::load_events() {
    events_.clear();

    while (!feof(file_)) {
        Event event;

        if (fread(&event.type, sizeof(event.type), 1, file_) != 1) break;
        if (fread(&event.timestamp_ns, sizeof(event.timestamp_ns), 1, file_) != 1) break;
        if (fread(&event.thread_id, sizeof(event.thread_id), 1, file_) != 1) break;
        if (fread(&event.data, sizeof(event.data), 1, file_) != 1) break;

        uint8_t has_label;
        if (fread(&has_label, 1, 1, file_) != 1) break;

        if (has_label) {
            uint32_t label_len;
            if (fread(&label_len, sizeof(label_len), 1, file_) != 1) break;

            // TODO: Implement string pool for labels
            std::vector<char> label_buf(label_len + 1);
            if (fread(label_buf.data(), 1, label_len, file_) != label_len) break;
            label_buf[label_len] = '\0';

            event.label = strdup(label_buf.data()); // TODO: Use a string pool instead of strdup to manage string lifetime properly
        } else {
            event.label = nullptr;
        }

        events_.push_back(event);
    }
}

} // namespace ggml_viz