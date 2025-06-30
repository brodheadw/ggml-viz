// test_trace_simple.cpp - Simple test of trace file reading
#include <iostream>
#include <fstream>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <trace_file.ggmlviz>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    std::cout << "=== Simple Trace File Test: " << filename << " ===" << std::endl;
    
    // Open file and check basic format
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cout << "❌ Cannot open file: " << filename << std::endl;
        return 1;
    }
    
    // Read magic header
    char magic[9] = {0};
    file.read(magic, 8);
    if (!file || strncmp(magic, "GGMLVIZ1", 8) != 0) {
        std::cout << "❌ Invalid magic header. Expected 'GGMLVIZ1', got: '" << magic << "'" << std::endl;
        return 1;
    }
    
    // Read version
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!file) {
        std::cout << "❌ Cannot read version" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Valid trace file!" << std::endl;
    std::cout << "📋 Magic: " << magic << std::endl;
    std::cout << "📋 Version: " << version << std::endl;
    
    // Get file size and data size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    size_t data_size = file_size - 12; // Header is 12 bytes
    
    std::cout << "📏 File size: " << file_size << " bytes" << std::endl;
    std::cout << "📏 Data size: " << data_size << " bytes" << std::endl;
    
    if (data_size == 0) {
        std::cout << "📊 No events recorded (empty trace)" << std::endl;
    } else {
        std::cout << "📊 Contains event data (" << data_size << " bytes)" << std::endl;
    }
    
    file.close();
    std::cout << "✅ Trace file format test completed!" << std::endl;
    
    return 0;
}