#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace ggml_viz {

/**
 * Ring buffer header for lock-free single-writer/multi-reader design
 */
struct RingHeader {
    std::atomic<uint32_t> head{0};    // Write position
    std::atomic<uint32_t> tail{0};    // Read position
    uint32_t capacity{0};             // Buffer capacity (must be power of 2)
    
    RingHeader() = default;
    
    // Explicitly disable copy constructor and assignment to prevent issues with atomics
    RingHeader(const RingHeader&) = delete;
    RingHeader& operator=(const RingHeader&) = delete;
};

/**
 * Abstract base class for cross-platform shared memory regions
 */
class SharedMemoryRegion {
public:
    virtual ~SharedMemoryRegion() = default;
    
    // Factory methods
    static std::unique_ptr<SharedMemoryRegion> create(const std::string& name, size_t size);
    static std::unique_ptr<SharedMemoryRegion> open(const std::string& name, size_t size);
    
    // Core interface
    virtual bool is_valid() const = 0;
    virtual void* get_address() const = 0;
    virtual size_t get_size() const = 0;
    
    // Ring buffer operations
    virtual bool write(const void* data, size_t nbytes) = 0;
    virtual bool read(void* data, size_t nbytes) = 0;
    virtual uint32_t available_space() const = 0;
    virtual uint32_t available_data() const = 0;
    
    // Convenience methods
    const RingHeader* header() const {
        return static_cast<const RingHeader*>(get_address());
    }
    
    void* data() const {
        return static_cast<char*>(get_address()) + sizeof(RingHeader);
    }
};

} // namespace ggml_viz