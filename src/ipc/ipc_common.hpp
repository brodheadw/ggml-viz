#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string_view>

namespace ggml_viz {

/**
 * Lockfree singlewriter / multireader ring buffer sitting in a shared
 * memory segment.  Layout is:
 * 
 *  RingHeader    sizeof(RingHeader) bytes (64byte aligned) 
 * $                                              
 *  event data&   capacity bytes (poweroftwo) <
 * 
 */
class SharedMemoryRegion {
public:
    struct RingHeader {
        /* Written by producer, read by consumers.  32bit wraparound; */
        std::atomic<uint32_t> head{0}; // next write offset (mod capacity)
        std::atomic<uint32_t> tail{0}; // next read offset  (mod capacity)
        uint32_t              capacity = 0; // size of data region in bytes
        uint32_t              _pad    = 0;  // keep 64bit alignment
    } __attribute__((aligned(64)));

    virtual ~SharedMemoryRegion() = default;

    /** Factory: create or open a named segment.  When `create` is true and the
     * segment already exists, implementation should truncate to `size_bytes`.
     */
    static std::unique_ptr<SharedMemoryRegion> create(std::string_view name,
                                                      size_t        size_bytes,
                                                      bool          create);

    /** Write `nbytes` from `data` into the ring.  Returns `false` if the buffer
     * does not have enough free space (caller can retry or drop the event). */
    virtual bool write(const void* data, size_t nbytes) = 0;

    /** Read up to `nbytes` into `dest`.  Returns the number of bytes copied
     * (may be less than `nbytes` if the buffer becomes empty). */
    virtual size_t read(void* dest, size_t nbytes) = 0;

    /** Raw pointer to the mapped memory (for testing / zerocopy). */
    virtual void* data() = 0;
    virtual const RingHeader* header() const = 0;
};

} // namespace ggml_viz