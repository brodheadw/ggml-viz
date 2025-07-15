/**
 * Windows shared memory implementation using CreateFileMappingW
 * 
 * This implementation uses Windows file mapping APIs to create named shared memory regions
 * that can be accessed across processes. The implementation follows the same interface
 * as the POSIX version but uses Windows-specific APIs.
 */

#include "ipc_common.hpp"
#include <windows.h>
#include <iostream>
#include <memory>
#include <string>
#include <codecvt>
#include <locale>

namespace ggml_viz {

class WindowsSharedMemory : public SharedMemoryRegion {
private:
    HANDLE mapping_handle_;
    void* addr_;
    size_t size_;
    std::wstring name_;
    bool is_creator_;

    // Convert UTF-8 string to wide string for Windows APIs
    std::wstring utf8_to_wide(const std::string& utf8_str) {
        if (utf8_str.empty()) return std::wstring();
        
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
        std::wstring result(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &result[0], size_needed);
        return result;
    }

public:
    WindowsSharedMemory(const std::string& name, size_t size, bool create)
        : mapping_handle_(INVALID_HANDLE_VALUE), addr_(nullptr), size_(size), is_creator_(create) {
        
        // Convert name to wide string and prepend Global\ for cross-session access
        name_ = L"Global\\ggml_viz_" + utf8_to_wide(name);
        
        if (create) {
            // Create new shared memory region
            mapping_handle_ = CreateFileMappingW(
                INVALID_HANDLE_VALUE,    // Use paging file
                NULL,                    // Default security
                PAGE_READWRITE,          // Read/write access
                0,                       // High-order DWORD of size
                static_cast<DWORD>(size), // Low-order DWORD of size
                name_.c_str()            // Name of mapping object
            );
            
            if (mapping_handle_ == NULL) {
                DWORD error = GetLastError();
                std::cerr << "ggml-viz: Failed to create file mapping: " << error << std::endl;
                return;
            }
            
            // Check if mapping already existed
            if (GetLastError() == ERROR_ALREADY_EXISTS) {
                std::cerr << "ggml-viz: Warning: Shared memory region already exists" << std::endl;
            }
        } else {
            // Open existing shared memory region
            mapping_handle_ = OpenFileMappingW(
                FILE_MAP_ALL_ACCESS,     // Read/write access
                FALSE,                   // Do not inherit handle
                name_.c_str()            // Name of mapping object
            );
            
            if (mapping_handle_ == NULL) {
                DWORD error = GetLastError();
                std::cerr << "ggml-viz: Failed to open file mapping: " << error << std::endl;
                return;
            }
        }
        
        // Map the shared memory into our address space
        addr_ = MapViewOfFile(
            mapping_handle_,         // Handle to mapping object
            FILE_MAP_ALL_ACCESS,     // Read/write access
            0,                       // High-order DWORD of offset
            0,                       // Low-order DWORD of offset
            size                     // Number of bytes to map
        );
        
        if (addr_ == nullptr) {
            DWORD error = GetLastError();
            std::cerr << "ggml-viz: Failed to map view of file: " << error << std::endl;
            CloseHandle(mapping_handle_);
            mapping_handle_ = INVALID_HANDLE_VALUE;
            return;
        }
        
        if (create) {
            // Initialize the ring buffer header
            auto* header = static_cast<RingHeader*>(addr_);
            new (header) RingHeader();  // Placement new to initialize atomics
            header->capacity = static_cast<uint32_t>(size - sizeof(RingHeader));
            
            // Ensure capacity is power of 2 for efficient masking
            uint32_t cap = header->capacity;
            if (cap == 0 || (cap & (cap - 1)) != 0) {
                std::cerr << "ggml-viz: Error: Ring buffer capacity must be power of 2, got " 
                         << cap << std::endl;
                close();
                return;
            }
        }
    }
    
    ~WindowsSharedMemory() {
        close();
    }
    
    bool is_valid() const override {
        return mapping_handle_ != INVALID_HANDLE_VALUE && addr_ != nullptr;
    }
    
    void* get_address() const override {
        return addr_;
    }
    
    size_t get_size() const override {
        return size_;
    }
    
    bool write(const void* data, size_t nbytes) override {
        if (!is_valid() || data == nullptr || nbytes == 0) {
            return false;
        }
        
        auto* hdr = static_cast<RingHeader*>(addr_);
        uint32_t head = hdr->head.load(std::memory_order_relaxed);
        uint32_t tail = hdr->tail.load(std::memory_order_acquire);
        uint32_t cap = hdr->capacity;
        
        // Calculate available space (leave one slot empty to distinguish full from empty)
        uint32_t free_space = (tail + cap - head - 1) & (cap - 1);
        
        if (nbytes > free_space) {
            return false; // Drop event if insufficient space
        }
        
        // Write data to ring buffer
        char* buffer = static_cast<char*>(addr_) + sizeof(RingHeader);
        uint32_t write_pos = head & (cap - 1);
        
        if (write_pos + nbytes <= cap) {
            // Single contiguous write
            memcpy(buffer + write_pos, data, nbytes);
        } else {
            // Split write (wrap around)
            uint32_t first_part = cap - write_pos;
            memcpy(buffer + write_pos, data, first_part);
            memcpy(buffer, static_cast<const char*>(data) + first_part, nbytes - first_part);
        }
        
        // Update head pointer with release semantics
        hdr->head.store(head + static_cast<uint32_t>(nbytes), std::memory_order_release);
        return true;
    }
    
    bool read(void* data, size_t nbytes) override {
        if (!is_valid() || data == nullptr || nbytes == 0) {
            return false;
        }
        
        auto* hdr = static_cast<RingHeader*>(addr_);
        uint32_t tail = hdr->tail.load(std::memory_order_relaxed);
        uint32_t head = hdr->head.load(std::memory_order_acquire);
        uint32_t cap = hdr->capacity;
        
        // Calculate available data
        uint32_t available = (head - tail) & (cap - 1);
        
        if (nbytes > available) {
            return false; // Not enough data available
        }
        
        // Read data from ring buffer
        char* buffer = static_cast<char*>(addr_) + sizeof(RingHeader);
        uint32_t read_pos = tail & (cap - 1);
        
        if (read_pos + nbytes <= cap) {
            // Single contiguous read
            memcpy(data, buffer + read_pos, nbytes);
        } else {
            // Split read (wrap around)
            uint32_t first_part = cap - read_pos;
            memcpy(data, buffer + read_pos, first_part);
            memcpy(static_cast<char*>(data) + first_part, buffer, nbytes - first_part);
        }
        
        // Update tail pointer with release semantics
        hdr->tail.store(tail + static_cast<uint32_t>(nbytes), std::memory_order_release);
        return true;
    }
    
    uint32_t available_space() const override {
        if (!is_valid()) return 0;
        
        auto* hdr = static_cast<RingHeader*>(addr_);
        uint32_t head = hdr->head.load(std::memory_order_relaxed);
        uint32_t tail = hdr->tail.load(std::memory_order_acquire);
        uint32_t cap = hdr->capacity;
        
        return (tail + cap - head - 1) & (cap - 1);
    }
    
    uint32_t available_data() const override {
        if (!is_valid()) return 0;
        
        auto* hdr = static_cast<RingHeader*>(addr_);
        uint32_t tail = hdr->tail.load(std::memory_order_relaxed);
        uint32_t head = hdr->head.load(std::memory_order_acquire);
        uint32_t cap = hdr->capacity;
        
        return (head - tail) & (cap - 1);
    }

private:
    void close() {
        if (addr_ != nullptr) {
            UnmapViewOfFile(addr_);
            addr_ = nullptr;
        }
        
        if (mapping_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(mapping_handle_);
            mapping_handle_ = INVALID_HANDLE_VALUE;
        }
    }
};

// Factory function implementation for Windows
std::unique_ptr<SharedMemoryRegion> SharedMemoryRegion::create(
    const std::string& name, size_t size) {
    
    auto region = std::make_unique<WindowsSharedMemory>(name, size, true);
    if (!region->is_valid()) {
        return nullptr;
    }
    return std::move(region);
}

std::unique_ptr<SharedMemoryRegion> SharedMemoryRegion::open(
    const std::string& name, size_t size) {
    
    auto region = std::make_unique<WindowsSharedMemory>(name, size, false);
    if (!region->is_valid()) {
        return nullptr;
    }
    return std::move(region);
}

} // namespace ggml_viz