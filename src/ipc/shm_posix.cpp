// ============================================================================
// shm_posix.cpp    POSIX mmap / shm_open implementation (Linux, macOS)
// ============================================================================

#if defined(__unix__) || defined(__APPLE__)

#include "ipc_common.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <string>
#include <stdexcept>
#include <algorithm>

namespace ggml_viz {

class PosixSharedMemory final : public SharedMemoryRegion {
public:
    ~PosixSharedMemory() override {
        if (addr_) {
            munmap(addr_, map_size_);
        }
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    bool write(const void* data, size_t nbytes) override;
    size_t read(void* dest, size_t nbytes) override;
    void* data() override { return addr_; }
    const RingHeader* header() const override { return static_cast<RingHeader*>(addr_); }

    static std::unique_ptr<SharedMemoryRegion> createImpl(std::string_view name,
                                                          size_t        size_bytes,
                                                          bool          create);

private:
    PosixSharedMemory() = default;
    int    fd_       = -1;
    void*  addr_     = nullptr;
    size_t map_size_ = 0;
};

//  factory --------------------------------------------------------------
std::unique_ptr<SharedMemoryRegion>
SharedMemoryRegion::create(std::string_view name, size_t size_bytes, bool create) {
    return PosixSharedMemory::createImpl(name, size_bytes, create);
}

std::unique_ptr<SharedMemoryRegion>
PosixSharedMemory::createImpl(std::string_view name, size_t size_bytes, bool create) {
    auto shm_name = std::string{"/"} + std::string{name};
    int flags = create ? (O_RDWR | O_CREAT) : O_RDWR;
    int fd    = shm_open(shm_name.c_str(), flags, 0666);
    if (fd < 0) {
        throw std::runtime_error{"shm_open failed: " + std::string{strerror(errno)}};
    }

    if (create) {
        // Ensure the segment is the requested size (header + data)
        size_t map_size = sizeof(RingHeader) + size_bytes;
        if (ftruncate(fd, static_cast<off_t>(map_size)) < 0) {
            close(fd);
            throw std::runtime_error{"ftruncate failed"};
        }
    }

    // Map the region
    size_t map_size = sizeof(RingHeader) + size_bytes;
    void* addr = mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        throw std::runtime_error{"mmap failed"};
    }

    // Initialise header when creating
    if (create) {
        auto* hdr = static_cast<RingHeader*>(addr);
        hdr->head.store(0, std::memory_order_relaxed);
        hdr->tail.store(0, std::memory_order_relaxed);
        hdr->capacity = static_cast<uint32_t>(size_bytes);
    }

    auto region          = std::unique_ptr<PosixSharedMemory>(new PosixSharedMemory);
    region->fd_          = fd;
    region->addr_        = addr;
    region->map_size_    = map_size;
    return region;
}

//  write ---------------------------------------------------------------
bool PosixSharedMemory::write(const void* data, size_t nbytes) {
    auto* hdr  = static_cast<RingHeader*>(addr_);
    uint32_t head = hdr->head.load(std::memory_order_relaxed);
    uint32_t tail = hdr->tail.load(std::memory_order_acquire);
    uint32_t cap  = hdr->capacity;

    uint32_t free_space = (tail + cap - head - 1) & (cap - 1);
    if (nbytes > free_space) {
        return false; // overwrite policy: drop event
    }

    uint8_t* base = static_cast<uint8_t*>(addr_) + sizeof(RingHeader);
    uint32_t offset = head;

    for (size_t i = 0; i < nbytes; ++i) {
        base[(offset + i) & (cap - 1)] = static_cast<const uint8_t*>(data)[i];
    }

    hdr->head.store((head + nbytes) & (cap - 1), std::memory_order_release);
    return true;
}

//  read ---------------------------------------------------------------
size_t PosixSharedMemory::read(void* dest, size_t nbytes) {
    auto* hdr  = static_cast<RingHeader*>(addr_);
    uint32_t head = hdr->head.load(std::memory_order_acquire);
    uint32_t tail = hdr->tail.load(std::memory_order_relaxed);
    uint32_t cap  = hdr->capacity;

    uint32_t available = (head + cap - tail) & (cap - 1);
    size_t to_copy = std::min<size_t>(nbytes, available);

    uint8_t* base = static_cast<uint8_t*>(addr_) + sizeof(RingHeader);
    for (size_t i = 0; i < to_copy; ++i) {
        static_cast<uint8_t*>(dest)[i] = base[(tail + i) & (cap - 1)];
    }

    hdr->tail.store((tail + to_copy) & (cap - 1), std::memory_order_release);
    return to_copy;
}

} // namespace ggml_viz

#endif // __unix__ || __APPLE__