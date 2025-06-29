// src/main.cpp
#include "frontend/imgui_app.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Simple command line argument parsing
    std::string trace_file;
    
    if (argc > 1) {
        trace_file = argv[1];
    }
    
    try {
        ggml_viz::ImGuiApp app;
        
        if (!trace_file.empty()) {
            app.load_trace_file(trace_file);
        }
        
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}